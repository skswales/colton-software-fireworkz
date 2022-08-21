/* cfbfnative.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2019 Stuart Swales */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) access functions */

/* SKS March 2014 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/cfbf.h"

#if WINDOWS

_Check_return_
static STATUS
file_error_set_from_hresult(
    _InVal_     HRESULT hresult,
    /**/        va_list * pArgs)
{
    STATUS status = STATUS_FAIL;
    LPERRORINFO errorinfo = NULL;

    if(SUCCEEDED(GetErrorInfo(0, &errorinfo)) && (NULL != errorinfo))
    {
        BSTR bstr;

        IErrorInfo_GetDescription(errorinfo, &bstr);

#if TSTR_IS_SBSTR
        {
        const UINT mbchars_CodePage = GetACP();
        TCHAR tstr_buf[1024];
        int multi_n;

        multi_n =
            WideCharToMultiByte(mbchars_CodePage, 0 /*dwFlags*/,
                                bstr, -1 /*strlen_with_NULLCH*/,
                                tstr_buf, elemof32(tstr_buf),
                                NULL, NULL);

        status = file_error_set(tstr_buf);
        } /*block */
#else
        status = file_error_set(bstr);
#endif

        IErrorInfo_Release(errorinfo);
    }
    else
    {
        PTSTR buffer = NULL;

        consume_bool(WrapOsBoolChecking(
            0 != FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                NULL,
                hresult,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &buffer,
                0,
                pArgs)));

        if(buffer) status = file_error_set(buffer);

        if(buffer) LocalFree(buffer);
    }

    return(status);
}

/* read a single stream into memory from the given CFBF storage file */

_Check_return_
extern STATUS
stg_cfbf_read_stream_from_storage(
    _In_z_      PCWSTR wstr_storage_filename,
    _In_z_      PCWSTR wstr_stream_name,
    _OutRef_    P_ARRAY_HANDLE p_h_data)
{
    STATUS status = STATUS_OK;

    *p_h_data = 0;

    if(SUCCEEDED(StgIsStorageFile(wstr_storage_filename)))
    {
        LPSTORAGE storage;
        HRESULT hresult = StgOpenStorage(wstr_storage_filename, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0L, &storage);

        if(SUCCEEDED(hresult))
        {
            LPSTREAM stream;

            hresult = IStorage_OpenStream(storage, wstr_stream_name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0L, &stream);

            if(SUCCEEDED(hresult))
            {
                STATSTG statstg;

                hresult = IStream_Stat(stream, &statstg, STATFLAG_DEFAULT);

                if(SUCCEEDED(hresult))
                {
                    ULONG file_size = statstg.cbSize.u.LowPart;
                    P_BYTE p_u8;
                    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
                    if(NULL != (p_u8 = al_array_alloc_BYTE(p_h_data, file_size, &array_init_block, &status)))
                    {
                        ULONG cbRead;

                        hresult = IStream_Read(stream, p_u8, file_size, &cbRead);

                        if(SUCCEEDED(hresult))
                            status = STATUS_DONE;

                        if(STATUS_DONE != status)
                            al_array_dispose(p_h_data);
                    }
                }

                IStream_Release(stream);
            }

            IStorage_Release(storage);
        }
    }

    return(status);
}

/* write memory to the file as a CFBF storage containing a single stream */

_Check_return_
extern STATUS
stg_cfbf_write_stream_in_storage(
    _In_z_      PCWSTR wstr_storage_filename,
    _In_z_      PCWSTR wstr_stream_name,
    _In_reads_bytes_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes)
{
    STATUS status;
    LPSTORAGE storage;
    DWORD_PTR pArgs[1];
    HRESULT hresult = StgCreateDocfile(wstr_storage_filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0L, &storage);

    pArgs[0] = (DWORD_PTR) tstr_empty_string; /* in case of error reporting needing %1 substitution - NB Fireworkz error reporting will append filename anyway */

    if(SUCCEEDED(hresult))
    {
        LPSTREAM stream;

        hresult = IStorage_CreateStream(storage, wstr_stream_name, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0L, 0L, &stream);

        if(SUCCEEDED(hresult))
        {
            DWORD bytes_written;

            hresult = IStream_Write(stream, p_data, n_bytes, &bytes_written);

            if(SUCCEEDED(hresult))
            {
                status = STATUS_DONE;
            }
            else
                status = file_error_set_from_hresult(hresult, (va_list *) pArgs);

            IStream_Release(stream);
        }
        else
            status = file_error_set_from_hresult(hresult, (va_list *) pArgs);

        IStorage_Release(storage);
    }
    else
        status = file_error_set_from_hresult(hresult, (va_list *) pArgs);

    return(status);
}

#endif /* WINDOWS */

/* end of cfbfnative.c */
