/*
 * Copyright 2005 Kai Blin
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "rpc.h"
#include "wincred.h"

#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

/***********************************************************************
 *              QueryCredentialsAttributesA
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryCredentialsAttributesA(
    PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    FIXME("%p, %lu, %p\n", phCredential, ulAttribute, pBuffer);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *              QueryCredentialsAttributesW
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryCredentialsAttributesW(
    PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    FIXME("%p, %lu, %p\n", phCredential, ulAttribute, pBuffer);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

struct sec_handle
{
    SecureProvider *krb;
    SecureProvider *ntlm;
    SecHandle       handle_krb;
    SecHandle       handle_ntlm;
};

#define WINE_NO_CACHED_CREDENTIALS 0x10000000

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
static SECURITY_STATUS SEC_ENTRY nego_AcquireCredentialsHandleW(
    SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
    PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
    PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry )
{
    static SEC_WCHAR ntlmW[] = {'N','T','L','M',0};
    static SEC_WCHAR kerberosW[] = {'K','e','r','b','e','r','o','s',0};
    SECURITY_STATUS ret = SEC_E_NO_CREDENTIALS;
    struct sec_handle *cred;
    SecurePackage *package;

    TRACE("%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p\n",
          debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
          pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    if (!pszPackage) return SEC_E_SECPKG_NOT_FOUND;
    if (!(cred = calloc( 1, sizeof(*cred) ))) return SEC_E_INSUFFICIENT_MEMORY;

    if ((package = SECUR32_findPackageW( kerberosW )))
    {
        ret = package->provider->fnTableW.AcquireCredentialsHandleW( pszPrincipal, kerberosW,
                fCredentialUse, pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, &cred->handle_krb, ptsExpiry );
        if (ret == SEC_E_OK) cred->krb = package->provider;
    }

    if ((package = SECUR32_findPackageW( ntlmW )))
    {
        ULONG cred_use = pAuthData ? fCredentialUse : fCredentialUse | WINE_NO_CACHED_CREDENTIALS;

        ret = package->provider->fnTableW.AcquireCredentialsHandleW( pszPrincipal, ntlmW,
                cred_use, pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, &cred->handle_ntlm, ptsExpiry );
        if (ret == SEC_E_OK) cred->ntlm = package->provider;
    }

    if (cred->krb || cred->ntlm)
    {
        phCredential->dwLower = (ULONG_PTR)cred;
        phCredential->dwUpper = 0;
        return SEC_E_OK;
    }

    free( cred );
    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleA
 */
static SECURITY_STATUS SEC_ENTRY nego_AcquireCredentialsHandleA(
    SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
    PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
    PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry )
{
    SECURITY_STATUS ret = SEC_E_INSUFFICIENT_MEMORY;
    SEC_WCHAR *package = NULL;
    SEC_WINNT_AUTH_IDENTITY_A *id = pAuthData;
    SEC_WINNT_AUTH_IDENTITY_W idW = {};

    TRACE("%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p\n",
          debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
          pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    if (pszPackage)
    {
        int package_len = MultiByteToWideChar( CP_ACP, 0, pszPackage, -1, NULL, 0 );
        if (!(package = malloc( package_len * sizeof(SEC_WCHAR) ))) return SEC_E_INSUFFICIENT_MEMORY;
        MultiByteToWideChar( CP_ACP, 0, pszPackage, -1, package, package_len );
    }
    if (id && id->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        idW.UserLength = MultiByteToWideChar( CP_ACP, 0, (const char *)id->User, id->UserLength, NULL, 0 );
        if (!(idW.User = malloc( idW.UserLength * sizeof(SEC_WCHAR) ))) goto done;
        MultiByteToWideChar( CP_ACP, 0, (const char *)id->User, id->UserLength, idW.User, idW.UserLength );

        idW.DomainLength = MultiByteToWideChar( CP_ACP, 0, (const char *)id->Domain, id->DomainLength, NULL, 0 );
        if (!(idW.Domain = malloc( idW.DomainLength * sizeof(SEC_WCHAR) ))) goto done;
        MultiByteToWideChar( CP_ACP, 0, (const char *)id->Domain, id->DomainLength, idW.Domain, idW.DomainLength );

        idW.PasswordLength = MultiByteToWideChar( CP_ACP, 0, (const char *)id->Password, id->PasswordLength, NULL, 0 );
        if (!(idW.Password = malloc( idW.PasswordLength * sizeof(SEC_WCHAR) ))) goto done;
        MultiByteToWideChar( CP_ACP, 0, (const char *)id->Password, id->PasswordLength, idW.Password, idW.PasswordLength );

        idW.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        pAuthData = &idW;
    }

    ret = nego_AcquireCredentialsHandleW( NULL, package, fCredentialUse, pLogonID, pAuthData,
                                          pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry );
done:
    free( package );
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextW
 */
static SECURITY_STATUS SEC_ENTRY nego_InitializeSecurityContextW(
    PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
    ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
    PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
    PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry )
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *handle = NULL, *ctxt, *new_ctxt = NULL, *cred = NULL;

    TRACE("%p, %p, %s, 0x%08lx, %lu, %lu, %p, %lu, %p, %p, %p, %p\n",
          phCredential, phContext, debugstr_w(pszTargetName), fContextReq,
          Reserved1, TargetDataRep, pInput, Reserved1, phNewContext, pOutput,
          pfContextAttr, ptsExpiry);

    if (phContext)
    {
        handle = ctxt = (struct sec_handle *)phContext->dwLower;
    }
    else if (phCredential)
    {
        handle = cred = (struct sec_handle *)phCredential->dwLower;
        if (!(new_ctxt = ctxt = calloc( 1, sizeof(*ctxt) ))) return SEC_E_INSUFFICIENT_MEMORY;
        ctxt->krb  = cred->krb;
        ctxt->ntlm = cred->ntlm;
    }
    if (!handle) return SEC_E_INVALID_HANDLE;

    if (handle->krb)
    {
        ret = handle->krb->fnTableW.InitializeSecurityContextW( phCredential ? &cred->handle_krb : NULL,
                phContext ? &ctxt->handle_krb : NULL, pszTargetName, fContextReq, Reserved1, TargetDataRep, pInput,
                Reserved2, phNewContext ? &ctxt->handle_krb : NULL, pOutput, pfContextAttr, ptsExpiry );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && phNewContext)
        {
            ctxt->ntlm = NULL;
            phNewContext->dwLower = (ULONG_PTR)ctxt;
            phNewContext->dwUpper = 0;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    if (ret != SEC_E_OK && ret != SEC_I_CONTINUE_NEEDED && handle->ntlm)
    {
        ret = handle->ntlm->fnTableW.InitializeSecurityContextW( phCredential ? &cred->handle_ntlm : NULL,
                phContext ? &ctxt->handle_ntlm : NULL, pszTargetName, fContextReq, Reserved1, TargetDataRep, pInput,
                Reserved2, phNewContext ? &ctxt->handle_ntlm : NULL, pOutput, pfContextAttr, ptsExpiry );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && phNewContext)
        {
            ctxt->krb = NULL;
            phNewContext->dwLower = (ULONG_PTR)ctxt;
            phNewContext->dwUpper = 0;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    free( new_ctxt );
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY nego_InitializeSecurityContextA(
    PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
    ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
    PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
    PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry )
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target = NULL;

    TRACE("%p, %p, %s, 0x%08lx, %lu, %lu, %p, %lu, %p, %p, %p, %p\n",
          phCredential, phContext, debugstr_a(pszTargetName), fContextReq,
          Reserved1, TargetDataRep, pInput, Reserved1, phNewContext, pOutput,
          pfContextAttr, ptsExpiry);

    if (pszTargetName)
    {
        int target_len = MultiByteToWideChar( CP_ACP, 0, pszTargetName, -1, NULL, 0 );
        if (!(target = malloc( target_len * sizeof(SEC_WCHAR) ))) return SEC_E_INSUFFICIENT_MEMORY;
        MultiByteToWideChar( CP_ACP, 0, pszTargetName, -1, target, target_len );
    }
    ret = nego_InitializeSecurityContextW( phCredential, phContext, target, fContextReq,
                                           Reserved1, TargetDataRep, pInput, Reserved2,
                                           phNewContext, pOutput, pfContextAttr, ptsExpiry );
    free( target );
    return ret;
}

/***********************************************************************
 *              AcceptSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY nego_AcceptSecurityContext(
    PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
    ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext,
    PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *handle = NULL, *ctxt, *new_ctxt = NULL, *cred = NULL;

    TRACE("%p, %p, %p, 0x%08lx, %lu, %p, %p, %p, %p\n", phCredential, phContext,
          pInput, fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
          ptsExpiry);

    if (phContext)
    {
        handle = ctxt = (struct sec_handle *)phContext->dwLower;
    }
    else if (phCredential)
    {
        handle = cred = (struct sec_handle *)phCredential->dwLower;
        if (!(new_ctxt = ctxt = calloc( 1, sizeof(*ctxt) ))) return SEC_E_INSUFFICIENT_MEMORY;
        ctxt->krb  = cred->krb;
        ctxt->ntlm = cred->ntlm;
    }
    if (!handle) return SEC_E_INVALID_HANDLE;

    if (handle->krb)
    {
        ret = handle->krb->fnTableW.AcceptSecurityContext( phCredential ? &cred->handle_krb : NULL,
            phContext ? &ctxt->handle_krb : NULL, pInput, fContextReq, TargetDataRep,
            phNewContext ? &ctxt->handle_krb : NULL, pOutput, pfContextAttr, ptsExpiry );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && phNewContext)
        {
            ctxt->ntlm = NULL;
            phNewContext->dwLower = (ULONG_PTR)ctxt;
            phNewContext->dwUpper = 0;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    if (ret != SEC_E_OK && ret != SEC_I_CONTINUE_NEEDED && handle->ntlm)
    {
        ret = handle->ntlm->fnTableW.AcceptSecurityContext( phCredential ? &cred->handle_ntlm : NULL,
            phContext ? &ctxt->handle_ntlm : NULL, pInput, fContextReq, TargetDataRep,
            phNewContext ? &ctxt->handle_ntlm : NULL, pOutput, pfContextAttr, ptsExpiry );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && phNewContext)
        {
            ctxt->krb = NULL;
            phNewContext->dwLower = (ULONG_PTR)ctxt;
            phNewContext->dwUpper = 0;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    free( new_ctxt );
    return ret;
}

/***********************************************************************
 *              CompleteAuthToken
 */
static SECURITY_STATUS SEC_ENTRY nego_CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;

    TRACE("%p %p\n", phContext, pToken);

    if (phContext) ret = SEC_E_UNSUPPORTED_FUNCTION;
    return ret;
}

/***********************************************************************
 *              DeleteSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY nego_DeleteSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p\n", phContext);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.DeleteSecurityContext( &ctxt->handle_krb );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.DeleteSecurityContext( &ctxt->handle_ntlm );
    }
    TRACE( "freeing %p\n", ctxt );
    free( ctxt );
    return ret;
}

/***********************************************************************
 *              ApplyControlToken
 */
static SECURITY_STATUS SEC_ENTRY nego_ApplyControlToken(PCtxtHandle phContext,
 PSecBufferDesc pInput)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;

    TRACE("%p %p\n", phContext, pInput);

    if (phContext) ret = SEC_E_UNSUPPORTED_FUNCTION;
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesW
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryContextAttributesW(
    PCtxtHandle phContext, ULONG ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, %lu, %p\n", phContext, ulAttribute, pBuffer);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.QueryContextAttributesW( &ctxt->handle_krb, ulAttribute, pBuffer );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.QueryContextAttributesW( &ctxt->handle_ntlm, ulAttribute, pBuffer );
    }
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesA
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryContextAttributesA(PCtxtHandle phContext,
    ULONG ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, %lu, %p\n", phContext, ulAttribute, pBuffer);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableA.QueryContextAttributesA( &ctxt->handle_krb, ulAttribute, pBuffer );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableA.QueryContextAttributesA( &ctxt->handle_ntlm, ulAttribute, pBuffer );
    }
    return ret;
}

/***********************************************************************
 *              ImpersonateSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY nego_ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;

    TRACE("%p\n", phContext);

    if (phContext) ret = SEC_E_UNSUPPORTED_FUNCTION;
    return ret;
}

/***********************************************************************
 *              RevertSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY nego_RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;

    TRACE("%p\n", phContext);

    if (phContext) ret = SEC_E_UNSUPPORTED_FUNCTION;
    return ret;
}

/***********************************************************************
 *              MakeSignature
 */
static SECURITY_STATUS SEC_ENTRY nego_MakeSignature(PCtxtHandle phContext,
    ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, 0x%08lx, %p, %lu\n", phContext, fQOP, pMessage, MessageSeqNo);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.MakeSignature( &ctxt->handle_krb, fQOP, pMessage, MessageSeqNo );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.MakeSignature( &ctxt->handle_ntlm, fQOP, pMessage, MessageSeqNo );
    }
    return ret;
}

/***********************************************************************
 *              VerifySignature
 */
static SECURITY_STATUS SEC_ENTRY nego_VerifySignature(PCtxtHandle phContext,
    PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, %p, %lu, %p\n", phContext, pMessage, MessageSeqNo, pfQOP);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.VerifySignature( &ctxt->handle_krb, pMessage, MessageSeqNo, pfQOP );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.VerifySignature( &ctxt->handle_ntlm, pMessage, MessageSeqNo, pfQOP );
    }
    return ret;
}

/***********************************************************************
 *             FreeCredentialsHandle
 */
static SECURITY_STATUS SEC_ENTRY nego_FreeCredentialsHandle(PCredHandle phCredential)
{
    struct sec_handle *cred;

    TRACE("%p\n", phCredential);

    if (!phCredential) return SEC_E_INVALID_HANDLE;

    cred = (struct sec_handle *)phCredential->dwLower;
    if (cred->krb) cred->krb->fnTableW.FreeCredentialsHandle( &cred->handle_krb );
    if (cred->ntlm) cred->ntlm->fnTableW.FreeCredentialsHandle( &cred->handle_ntlm );

    free( cred );
    return SEC_E_OK;
}

/***********************************************************************
 *             EncryptMessage
 */
static SECURITY_STATUS SEC_ENTRY nego_EncryptMessage(PCtxtHandle phContext,
    ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, 0x%08lx, %p, %lu\n", phContext, fQOP, pMessage, MessageSeqNo);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.EncryptMessage( &ctxt->handle_krb, fQOP, pMessage, MessageSeqNo );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.EncryptMessage( &ctxt->handle_ntlm, fQOP, pMessage, MessageSeqNo );
    }
    return ret;
}

/***********************************************************************
 *             DecryptMessage
 */
static SECURITY_STATUS SEC_ENTRY nego_DecryptMessage(PCtxtHandle phContext,
    PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE("%p, %p, %lu, %p\n", phContext, pMessage, MessageSeqNo, pfQOP);

    if (!phContext) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)phContext->dwLower;
    if (ctxt->krb)
    {
        ret = ctxt->krb->fnTableW.DecryptMessage( &ctxt->handle_krb, pMessage, MessageSeqNo, pfQOP );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->fnTableW.DecryptMessage( &ctxt->handle_ntlm, pMessage, MessageSeqNo, pfQOP );
    }
    return ret;
}

static const SecurityFunctionTableA negoTableA = {
    1,
    NULL,                               /* EnumerateSecurityPackagesA */
    nego_QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    nego_AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    nego_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,                               /* Reserved2 */
    nego_InitializeSecurityContextA,    /* InitializeSecurityContextA */
    nego_AcceptSecurityContext,         /* AcceptSecurityContext */
    nego_CompleteAuthToken,             /* CompleteAuthToken */
    nego_DeleteSecurityContext,         /* DeleteSecurityContext */
    nego_ApplyControlToken,             /* ApplyControlToken */
    nego_QueryContextAttributesA,       /* QueryContextAttributesA */
    nego_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    nego_RevertSecurityContext,         /* RevertSecurityContext */
    nego_MakeSignature,                 /* MakeSignature */
    nego_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoA */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextA */
    NULL,   /* AddCredentialsA */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    nego_EncryptMessage,                /* EncryptMessage */
    nego_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesA */
};

static const SecurityFunctionTableW negoTableW = {
    1,
    NULL,                               /* EnumerateSecurityPackagesW */
    nego_QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    nego_AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    nego_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,                               /* Reserved2 */
    nego_InitializeSecurityContextW,    /* InitializeSecurityContextW */
    nego_AcceptSecurityContext,         /* AcceptSecurityContext */
    nego_CompleteAuthToken,             /* CompleteAuthToken */
    nego_DeleteSecurityContext,         /* DeleteSecurityContext */
    nego_ApplyControlToken,             /* ApplyControlToken */
    nego_QueryContextAttributesW,       /* QueryContextAttributesW */
    nego_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    nego_RevertSecurityContext,         /* RevertSecurityContext */
    nego_MakeSignature,                 /* MakeSignature */
    nego_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoW */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextW */
    NULL,   /* AddCredentialsW */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    nego_EncryptMessage,                /* EncryptMessage */
    nego_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesW */
};

#define NEGO_MAX_TOKEN 12000

static WCHAR nego_name_W[] = {'N','e','g','o','t','i','a','t','e',0};
static char nego_name_A[] = "Negotiate";

static WCHAR negotiate_comment_W[] =
    {'M','i','c','r','o','s','o','f','t',' ','P','a','c','k','a','g','e',' ',
     'N','e','g','o','t','i','a','t','o','r',0};
static CHAR negotiate_comment_A[] = "Microsoft Package Negotiator";

#define CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED | \
    SECPKG_FLAG_EXTENDED_ERROR | \
    SECPKG_FLAG_IMPERSONATION  | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_GSS_COMPATIBLE    | \
    SECPKG_FLAG_LOGON             | \
    SECPKG_FLAG_RESTRICTED_TOKENS )

void SECUR32_initNegotiateSP(void)
{
    SecureProvider *provider = SECUR32_addProvider(&negoTableA, &negoTableW, NULL);

    const SecPkgInfoW infoW = {CAPS, 1, RPC_C_AUTHN_GSS_NEGOTIATE, NEGO_MAX_TOKEN,
                               nego_name_W, negotiate_comment_W};
    const SecPkgInfoA infoA = {CAPS, 1, RPC_C_AUTHN_GSS_NEGOTIATE, NEGO_MAX_TOKEN,
                               nego_name_A, negotiate_comment_A};
    SECUR32_addPackages(provider, 1L, &infoA, &infoW);
}
