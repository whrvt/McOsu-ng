#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

//#include "MD5Hash.h"
//#include "types.h"

#ifdef _MSC_VER
#define always_inline_attr __forceinline
#elif defined(__GNUC__)
#define always_inline_attr [[gnu::always_inline]] inline
#else
#define always_inline_attr
#endif

class ByteBufferedFile {
   private:
    static constexpr const size_t READ_BUFFER_SIZE{512ULL * 4096};
    static constexpr const size_t WRITE_BUFFER_SIZE{512ULL * 4096};

   public:
    class Reader {
       public:
        Reader(const std::filesystem::path &path);
        ~Reader() = default;

        Reader &operator=(const Reader &) = delete;
        Reader &operator=(Reader &&) = delete;
        Reader(const Reader &) = delete;
        Reader(Reader &&) = delete;

        // always_inline is a 2x speedup here
        [[nodiscard]] always_inline_attr size_t readBytes(uint8_t *out, size_t len) {
            if(m_bErrorFlag || !m_inFile.is_open()) {
                if(out != nullptr) {
                    memset(out, 0, len);
                }
                return 0;
            }

            if(len > READ_BUFFER_SIZE) {
                this->setError("Attempted to read " + std::to_string(len) + " bytes (exceeding m_fileBuf size " +
                                std::to_string(READ_BUFFER_SIZE) + ")");
                if(out != nullptr) {
                    memset(out, 0, len);
                }
                return 0;
            }

            // make sure the ring buffer has enough data
            if(m_iBufferedBytes < len) {
                // calculate available space for reading more data
                size_t available_space = READ_BUFFER_SIZE - m_iBufferedBytes;
                size_t bytes_to_read = available_space;

                if(m_iWritePos + bytes_to_read <= READ_BUFFER_SIZE) {
                    // no wrap needed, read directly
                    m_inFile.read(reinterpret_cast<char *>(m_ringBuf.data() + m_iWritePos), bytes_to_read);
                    size_t bytes_read = m_inFile.gcount();
                    m_iWritePos = (m_iWritePos + bytes_read) % READ_BUFFER_SIZE;
                    m_iBufferedBytes += bytes_read;
                } else {
                    // wrap needed, read in two parts
                    size_t first_part = READ_BUFFER_SIZE - m_iWritePos;
                    m_inFile.read(reinterpret_cast<char *>(m_ringBuf.data() + m_iWritePos), first_part);
                    size_t bytes_read = m_inFile.gcount();

                    if(bytes_read == first_part && bytes_to_read > first_part) {
                        size_t second_part = bytes_to_read - first_part;
                        m_inFile.read(reinterpret_cast<char *>(m_ringBuf.data()), second_part);
                        size_t second_read = m_inFile.gcount();
                        bytes_read += second_read;
                        m_iWritePos = second_read;
                    } else {
                        m_iWritePos = (m_iWritePos + bytes_read) % READ_BUFFER_SIZE;
                    }

                    m_iBufferedBytes += bytes_read;
                }
            }

            if(m_iBufferedBytes < len) {
                // couldn't read enough data
                if(out != nullptr) {
                    memset(out, 0, len);
                }
                return 0;
            }

            // read from ring buffer
            if(out != nullptr) {
                if(m_iReadPos + len <= READ_BUFFER_SIZE) {
                    // no wrap needed
                    memcpy(out, m_ringBuf.data() + m_iReadPos, len);
                } else {
                    // wrap needed
                    size_t first_part = std::min(len, READ_BUFFER_SIZE - m_iReadPos);
                    size_t second_part = len - first_part;

                    memcpy(out, m_ringBuf.data() + m_iReadPos, first_part);
                    memcpy(out + first_part, m_ringBuf.data(), second_part);
                }
            }

            m_iReadPos = (m_iReadPos + len) % READ_BUFFER_SIZE;
            m_iBufferedBytes -= len;
            m_iTotalPos += len;

            return len;
        }

        template <typename T>
        [[nodiscard]] T read() {
            static_assert(sizeof(T) < READ_BUFFER_SIZE);

            T result;
            if((this->readBytes(reinterpret_cast<uint8_t *>(&result), sizeof(T))) != sizeof(T)) {
                memset(&result, 0, sizeof(T));
            }
            return result;
        }

        always_inline_attr void skipBytes(uint32_t n) {
            if(m_bErrorFlag || !m_inFile.is_open()) {
                return;
            }

            // if we can skip entirely within the buffered data
            if(n <= m_iBufferedBytes) {
                m_iReadPos = (m_iReadPos + n) % READ_BUFFER_SIZE;
                m_iBufferedBytes -= n;
                m_iTotalPos += n;
                return;
            }

            // we need to skip more than what's buffered
            uint32_t skip_from_buffer = m_iBufferedBytes;
            uint32_t skip_from_file = n - skip_from_buffer;

            // skip what's in the m_fileBuf
            m_iTotalPos += skip_from_buffer;

            // seek in the file to skip the rest
            m_inFile.seekg(skip_from_file, std::ios::cur);
            if(m_inFile.fail()) {
                this->setError("Failed to seek " + std::to_string(skip_from_file) + " bytes");
                return;
            }

            m_iTotalPos += skip_from_file;

            // since we've moved past buffered data, reset m_fileBuf state
            m_iReadPos = 0;
            m_iWritePos = 0;
            m_iBufferedBytes = 0;
        }

        template <typename T>
        void skip() {
            static_assert(sizeof(T) < READ_BUFFER_SIZE);
            this->skipBytes(sizeof(T));
        }

        [[nodiscard]] constexpr bool good() const { return !m_bErrorFlag; }
        [[nodiscard]] constexpr std::string_view error() const { return m_sLastError; }

        [[nodiscard]] constexpr size_t getTotalSize() const { return m_iTotalSize; }
        [[nodiscard]] constexpr size_t getTotalPos() const { return m_iTotalPos; }

        //[[nodiscard]] MD5Hash read_hash();
        [[nodiscard]] std::string readString();
        [[nodiscard]] uint32_t readULEB128();

        void skipString();

       private:
        size_t m_iTotalSize{0};
        size_t m_iTotalPos{0};

        void setError(const std::string &error_msg);

        std::ifstream m_inFile;

        std::vector<uint8_t> m_ringBuf;
        size_t m_iReadPos{0};        // current read position in ring buffer
        size_t m_iWritePos{0};       // current write position in ring buffer
        size_t m_iBufferedBytes{0};  // amount of data currently buffered

        bool m_bErrorFlag{false};
        std::string m_sLastError;
    };

    class Writer {
       public:
        Writer(const std::filesystem::path &path);
        ~Writer();

        Writer &operator=(const Writer &) = delete;
        Writer &operator=(Writer &&) = delete;
        Writer(const Writer &) = delete;
        Writer(Writer &&) = delete;

        [[nodiscard]] bool good() const { return !m_bErrorFlag; }
        [[nodiscard]] std::string_view error() const { return m_sLastError; }

        void flush();
        void writeBytes(uint8_t *bytes, size_t n);
        //void write_hash(MD5Hash hash);
        void writeString(std::string str);
        void writeULEB128(uint32_t num);

        template <typename T>
        void write(T t) {
            this->writeBytes(reinterpret_cast<uint8_t *>(&t), sizeof(T));
        }

       private:
        void setError(const std::string &error_msg);

        std::filesystem::path m_outFilePath;
        std::filesystem::path m_tmpFilePath;
        std::ofstream m_outFile;

        std::vector<uint8_t> m_ringBuf;
        size_t m_iFilePos{0};
        bool m_bErrorFlag{false};
        std::string m_sLastError;
    };

    static void copy(const std::filesystem::path &from_path, const std::filesystem::path &to_path);
};
