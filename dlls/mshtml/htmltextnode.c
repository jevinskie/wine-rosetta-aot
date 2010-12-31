/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "mshtml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLDOMTextNode {
    HTMLDOMNode node;
    const IHTMLDOMTextNodeVtbl   *lpIHTMLDOMTextNodeVtbl;

    nsIDOMText *nstext;
};

#define HTMLTEXT(x)  (&(x)->lpIHTMLDOMTextNodeVtbl)

#define HTMLTEXT_THIS(iface) DEFINE_THIS(HTMLDOMTextNode, IHTMLDOMTextNode, iface)

static HRESULT WINAPI HTMLDOMTextNode_QueryInterface(IHTMLDOMTextNode *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLDOMTextNode_AddRef(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLDOMTextNode_Release(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfoCount(IHTMLDOMTextNode *iface, UINT *pctinfo)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfo(IHTMLDOMTextNode *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetIDsOfNames(IHTMLDOMTextNode *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMTextNode_Invoke(IHTMLDOMTextNode *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMTextNode_put_data(IHTMLDOMTextNode *iface, BSTR v)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_data(IHTMLDOMTextNode *iface, BSTR *p)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_toString(IHTMLDOMTextNode *iface, BSTR *String)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_length(IHTMLDOMTextNode *iface, LONG *p)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    PRUint32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMText_GetLength(This->nstext, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08x\n", nsres);

    *p = length;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMTextNode_splitText(IHTMLDOMTextNode *iface, LONG offset, IHTMLDOMNode **pRetNode)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, offset, pRetNode);
    return E_NOTIMPL;
}

#undef HTMLTEXT_THIS

static const IHTMLDOMTextNodeVtbl HTMLDOMTextNodeVtbl = {
    HTMLDOMTextNode_QueryInterface,
    HTMLDOMTextNode_AddRef,
    HTMLDOMTextNode_Release,
    HTMLDOMTextNode_GetTypeInfoCount,
    HTMLDOMTextNode_GetTypeInfo,
    HTMLDOMTextNode_GetIDsOfNames,
    HTMLDOMTextNode_Invoke,
    HTMLDOMTextNode_put_data,
    HTMLDOMTextNode_get_data,
    HTMLDOMTextNode_toString,
    HTMLDOMTextNode_get_length,
    HTMLDOMTextNode_splitText
};

static inline HTMLDOMTextNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, node);
}

static HRESULT HTMLDOMTextNode_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IHTMLDOMTextNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMTextNode %p)\n", This, ppv);
        *ppv = HTMLTEXT(This);
    }else {
        return HTMLDOMNode_QI(&This->node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLDOMTextNode_destructor(HTMLDOMNode *iface)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    if(This->nstext)
        IHTMLDOMTextNode_Release(This->nstext);

    HTMLDOMNode_destructor(&This->node);
}

static HRESULT HTMLDOMTextNode_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);
    HRESULT hres;

    hres = HTMLDOMTextNode_Create(This->node.doc, nsnode, ret);
    if(FAILED(hres))
        return hres;

    IHTMLDOMNode_AddRef(&(*ret)->IHTMLDOMNode_iface);
    return S_OK;
}

static const NodeImplVtbl HTMLDOMTextNodeImplVtbl = {
    HTMLDOMTextNode_QI,
    HTMLDOMTextNode_destructor,
    HTMLDOMTextNode_clone
};

static const tid_t HTMLDOMTextNode_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLDOMTextNode_tid,
    0
};
static dispex_static_data_t HTMLDOMTextNode_dispex = {
    NULL,
    DispHTMLDOMTextNode_tid,
    0,
    HTMLDOMTextNode_iface_tids
};

HRESULT HTMLDOMTextNode_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLDOMNode **node)
{
    HTMLDOMTextNode *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->node.vtbl = &HTMLDOMTextNodeImplVtbl;
    ret->lpIHTMLDOMTextNodeVtbl = &HTMLDOMTextNodeVtbl;

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMText, (void**)&ret->nstext);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMText iface: %08x\n", nsres);
        heap_free(ret);
        return E_FAIL;
    }

    init_dispex(&ret->node.dispex, (IUnknown*)HTMLTEXT(ret), &HTMLDOMTextNode_dispex);
    HTMLDOMNode_Init(doc, &ret->node, nsnode);

    *node = &ret->node;
    return S_OK;
}
