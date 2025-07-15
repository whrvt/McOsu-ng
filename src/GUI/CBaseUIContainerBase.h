/*
 * CBaseUIContainerBase.h
 *
 *  Created on: May 31, 2017
 *      Author: Psy
 */

#pragma once
#ifndef GUI_CBASEUICONTAINERBASE_H_
#define GUI_CBASEUICONTAINERBASE_H_

#define CONTAINER_BODY(T) ELEMENT_BODY(T)\
	\
	T *addElement(CBaseUIElement *element, bool back=false) override {CBaseUIContainerBase::addElement(element, back); return this;} \
	T *addElement(std::shared_ptr<CBaseUIElement> element, bool back=false) override {CBaseUIContainerBase::addElement(element, back); return this;} \
	T *insertElement(CBaseUIElement *element, CBaseUIElement *index, bool back=false) override {CBaseUIContainerBase::insertElement(element, index, back); return this;} \
	T *insertElement(std::shared_ptr<CBaseUIElement> element, CBaseUIElement *index, bool back=false) override {CBaseUIContainerBase::insertElement(element, index, back); return this;} \
	T *insertElement(CBaseUIElement *element, std::shared_ptr<CBaseUIElement> index, bool back=false) override {CBaseUIContainerBase::insertElement(element, index, back); return this;} \
	T *insertElement(std::shared_ptr<CBaseUIElement> element, std::shared_ptr<CBaseUIElement> index, bool back=false) override {CBaseUIContainerBase::insertElement(element, index, back); return this;} \
	T *setClipping(bool clipping) override {CBaseUIContainerBase::setClipping(clipping); return this;}

#include "CBaseUIElement.h"
#include "cbase.h"

class CBaseUIContainerBase : public CBaseUIElement
{
public:

	CBaseUIContainerBase(UString name="");
	~CBaseUIContainerBase() override;

	ELEMENT_BODY(CBaseUIContainerBase);

	virtual CBaseUIContainerBase *addElement(CBaseUIElement *element, bool back=false);
	virtual CBaseUIContainerBase *addElement(std::shared_ptr<CBaseUIElement> element, bool back=false);
	virtual CBaseUIContainerBase *insertElement(CBaseUIElement *element, CBaseUIElement *index, bool back=false);
	virtual CBaseUIContainerBase *insertElement(std::shared_ptr<CBaseUIElement> element, CBaseUIElement *index, bool back=false);
	virtual CBaseUIContainerBase *insertElement(CBaseUIElement *element, std::shared_ptr<CBaseUIElement> index, bool back=false);
	virtual CBaseUIContainerBase *insertElement(std::shared_ptr<CBaseUIElement> element, std::shared_ptr<CBaseUIElement> index, bool back=false);

	virtual void removeElement(CBaseUIElement *element);
	virtual void removeElement(std::shared_ptr<CBaseUIElement> element);

	virtual CBaseUIContainerBase *setClipping(bool clipping) {m_bClipping = clipping; return this;}

	CBaseUIElement *getElementByName(const UString& name, bool searchNestedContainers=false);
	std::shared_ptr<CBaseUIElement> getElementSharedByName(const UString& name, bool searchNestedContainers=false);
	std::vector<CBaseUIElement*> getAllElements();
	inline std::vector<std::shared_ptr<CBaseUIElement>> getAllElementsShared(){return m_vElements;}
	inline std::vector<std::shared_ptr<CBaseUIElement>> *getAllElementsReference(){return &m_vElements;}

	void draw() override;
	void drawDebug(Color) {;}
	void update() override;

	virtual void empty();
	// inspection
	CBASE_UI_TYPE(CBaseUIContainerBase, CONTAINERBASE, CBaseUIElement)
protected:
	// events
	virtual void updateElement(CBaseUIElement * /*element*/) {;}

	bool m_bClipping;
	std::vector<std::shared_ptr<CBaseUIElement>> m_vElements;

};

#endif /* GUI_CBASEUICONTAINERBASE_H_ */
