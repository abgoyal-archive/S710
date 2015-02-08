/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#define XLOG_TAG "WebCore/webkit_plugin"

#include "config.h"

#include "Page.h"
#include "PluginMainThreadScheduler.h"
#include "PluginView.h"
#include "npruntime_internal.h"
#include <cutils/xlog.h>

using namespace WebCore;

// The plugin view is always the ndata of the instance,. Sometimes, plug-ins will call an instance-specific function
// with a NULL instance. To workaround this, call the last plug-in view that made a call to a plug-in.
// Currently, the current plug-in view is only set before NPP_New in PluginView::start.
// This specifically works around Flash and Shockwave. When we call NPP_New, they call NPN_Useragent with a NULL instance.
static PluginView* pluginViewForInstance(NPP instance)
{
    if (instance && instance->ndata)
        return static_cast<PluginView*>(instance->ndata);
    return PluginView::currentPluginView();
}

void* NPN_MemAlloc(uint32_t size)
{
    return malloc(size);
}

void NPN_MemFree(void* ptr)
{
    free(ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
    // Do nothing
    return 0;
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
    Page::refreshPlugins(reloadPages);
}

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
    return NPERR_STREAM_NOT_SEEKABLE;
}

NPError NPN_GetURLNotify(NPP instance, const char* url, const char* target, void* notifyData)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->getURLNotify(url, target, notifyData);
}

NPError NPN_GetURL(NPP instance, const char* url, const char* target)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->getURL(url, target);
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* target, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->postURLNotify(url, target, len, buf, file, notifyData);
}

NPError NPN_PostURL(NPP instance, const char* url, const char* target, uint32_t len, const char* buf, NPBool file)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->postURL(url, target, len, buf, file);
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->newStream(type, target, stream);
}

int32_t NPN_Write(NPP instance, NPStream* stream, int32_t len, void* buffer)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return 0;
    return view->write(stream, len, buffer);
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
    return view->destroyStream(stream, reason);
}

const char* NPN_UserAgent(NPP instance)
{
    PluginView* view = pluginViewForInstance(instance);

     if (!view)
         return PluginView::userAgentStatic();
 
    return view->userAgent();
}

void NPN_Status(NPP instance, const char* message)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->status(message);
}

void NPN_InvalidateRect(NPP instance, NPRect* invalidRect)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->invalidateRect(invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->invalidateRegion(invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->forceRedraw();
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void* value)
{
    PluginView* view = pluginViewForInstance(instance);

     if (!view)
         return PluginView::getValueStatic(variable, value);

    return pluginViewForInstance(instance)->getValue(variable, value);
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void* value)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return NPERR_INVALID_INSTANCE_ERROR;
   return view->setValue(variable, value);
}

void* NPN_GetJavaEnv()
{
    // Unsupported
    return 0;
}

void* NPN_GetJavaPeer(NPP instance)
{
    // Unsupported
    return 0;
}

void NPN_PushPopupsEnabledState(NPP instance, NPBool enabled)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->pushPopupsEnabledState(enabled);
}

void NPN_PopPopupsEnabledState(NPP instance)
{
    PluginView* view = pluginViewForInstance(instance);
    if(!view)
       return;
    view->popPopupsEnabledState();
}

extern "C" typedef void PluginThreadAsyncCallFunction(void*);
void NPN_PluginThreadAsyncCall(NPP instance, PluginThreadAsyncCallFunction func, void* userData)
{
    // Callback function type only differs from MainThreadFunction by being extern "C", which doesn't affect calling convention on any compilers we use.
    PluginMainThreadScheduler::scheduler().scheduleCall(instance, reinterpret_cast<PluginMainThreadScheduler::MainThreadFunction*>(func), userData);
}

NPError NPN_GetValueForURL(NPP instance, NPNURLVariable variable, const char* url, char** value, uint32_t* len)
{
    return pluginViewForInstance(instance)->getValueForURL(variable, url, value, len);
}

NPError NPN_SetValueForURL(NPP instance, NPNURLVariable variable, const char* url, const char* value, uint32_t len)
{
    return pluginViewForInstance(instance)->setValueForURL(variable, url, value, len);
}

NPError NPN_GetAuthenticationInfo(NPP instance, const char* protocol, const char* host, int32_t port, const char* scheme, const char* realm, char** username, uint32_t* ulen, char** password, uint32_t* plen)
{
    return pluginViewForInstance(instance)->getAuthenticationInfo(protocol, host, port, scheme, realm, username, ulen, password, plen);
}

#ifdef PLUGIN_SCHEDULE_TIMER
uint32_t NPN_ScheduleTimer(NPP instance, uint32_t interval, NPBool repeat,
                         void (*timerFunc)(NPP npp, uint32_t timerID))
{
    PluginView* view = pluginViewForInstance(instance);
    XLOGD2("NPN_ScheduleTimer, instance=%p, interval=%u, repeat=%s, view=%p", instance, interval, repeat!=0?"true":"false", view);
    if(!view)
       return 0;
    return view->scheduleTimer(instance, interval,
                                                        repeat != 0, timerFunc);
}

void NPN_UnscheduleTimer(NPP instance, uint32_t timerID)
{
    PluginView* view = pluginViewForInstance(instance);
    XLOGD2("NPN_UnscheduleTimer, instance=%p, timerID=%u, view=%p", instance, timerID, view);
    if(!view)
       return;
    view->unscheduleTimer(instance, timerID);
}
#endif
