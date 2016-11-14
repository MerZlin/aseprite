// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/resources_listbox.h"

#include "app/res/resource.h"
#include "app/res/resources_loader.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

//////////////////////////////////////////////////////////////////////
// ResourceListItem

ResourceListItem::ResourceListItem(Resource* resource)
  : ListItem(resource->name()), m_resource(resource)
{
}

bool ResourceListItem::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseLeaveMessage:
    case kMouseEnterMessage:
      invalidate();
      break;
  }
  return ListItem::onProcessMessage(msg);
}

void ResourceListItem::onPaint(PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();
  gfx::Color bgcolor, fgcolor;

  if (isSelected()) {
    bgcolor = theme->colors.listitemSelectedFace();
    fgcolor = theme->colors.listitemSelectedText();
  }
  else {
    bgcolor = theme->colors.listitemNormalFace();
    fgcolor = theme->colors.listitemNormalText();
  }

  g->fillRect(bgcolor, bounds);

  static_cast<ResourcesListBox*>(parent())->
    paintResource(g, bounds, m_resource);

  g->drawString(text(), fgcolor, gfx::ColorNone,
                gfx::Point(
                  bounds.x + 2*guiscale(),
                  bounds.y + bounds.h/2 - g->measureUIString(text()).h/2));
}

void ResourceListItem::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(
    static_cast<ResourcesListBox*>(parent())->
    resourceSizeHint(m_resource));
}

//////////////////////////////////////////////////////////////////////
// ResourcesListBox::LoadingItem

class ResourcesListBox::LoadingItem : public ListItem {
public:
  LoadingItem()
    : ListItem("Loading")
    , m_state(0) {
  }

  void makeProgress() {
    std::string text = "Loading ";

    switch ((++m_state) % 4) {
      case 0: text += "/"; break;
      case 1: text += "-"; break;
      case 2: text += "\\"; break;
      case 3: text += "|"; break;
    }

    setText(text);
  }

private:
  int m_state;
};

//////////////////////////////////////////////////////////////////////
// ResourcesListBox

ResourcesListBox::ResourcesListBox(ResourcesLoader* resourcesLoader)
  : m_resourcesLoader(resourcesLoader)
  , m_resourcesTimer(100)
  , m_loadingItem(NULL)
{
  m_resourcesTimer.Tick.connect(base::Bind<void>(&ResourcesListBox::onTick, this));
}

Resource* ResourcesListBox::selectedResource()
{
  if (ResourceListItem* listItem = dynamic_cast<ResourceListItem*>(getSelectedChild()))
    return listItem->resource();
  else
    return NULL;
}

void ResourcesListBox::paintResource(Graphics* g, gfx::Rect& bounds, Resource* resource)
{
  onPaintResource(g, bounds, resource);
}

gfx::Size ResourcesListBox::resourceSizeHint(Resource* resource)
{
  gfx::Size pref(0, 0);
  onResourceSizeHint(resource, pref);
  return pref;
}

bool ResourcesListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      m_resourcesTimer.start();
      break;
    }

  }
  return ListBox::onProcessMessage(msg);
}

void ResourcesListBox::onChange()
{
  Resource* resource = selectedResource();
  if (resource)
    onResourceChange(resource);
}

ResourceListItem* ResourcesListBox::onCreateResourceItem(Resource* resource)
{
  return new ResourceListItem(resource);
}

void ResourcesListBox::onTick()
{
  if (m_resourcesLoader == NULL) {
    stop();
    return;
  }

  if (!m_loadingItem) {
    m_loadingItem = new LoadingItem;
    addChild(m_loadingItem);
  }
  m_loadingItem->makeProgress();

  base::UniquePtr<Resource> resource;
  std::string name;

  if (!m_resourcesLoader->next(resource)) {
    if (m_resourcesLoader->isDone()) {
      stop();
    }
    return;
  }

  base::UniquePtr<ResourceListItem> listItem(onCreateResourceItem(resource));
  insertChild(getItemsCount()-1, listItem);
  layout();

  if (View* view = View::getView(this))
    view->updateView();

  resource.release();
  listItem.release();
}

void ResourcesListBox::stop()
{
  m_resourcesTimer.stop();

  if (m_loadingItem) {
    removeChild(m_loadingItem);
    layout();

    delete m_loadingItem;
    m_loadingItem = nullptr;

    if (View* view = View::getView(this))
      view->updateView();
  }
}

} // namespace app
