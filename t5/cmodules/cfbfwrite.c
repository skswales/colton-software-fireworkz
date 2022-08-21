/* cfbfwrite.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) access functions */

/* Copyright (C) 2014-2016 Stuart Swales */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/cfbf.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

static void
patch_header_common(
    _Inout_bytecap_c_(0x600) P_StructuredStorageHeader p_structured_storage_header,
    _In_z_      PCTSTR stream_name,
    _InVal_     U32 n_bytes)
{
    const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
    P_StructuredStorageDirectoryEntry file_dir;

    assert(1 == p_structured_storage_header->_sectDirStart);
    file_dir = PtrAddBytes(P_StructuredStorageDirectoryEntry, (p_structured_storage_header + 1), 1 * sector_size);
    assert(CFBF_STGTY_ROOT == file_dir->_mse);

    file_dir++; /* Skip Root Entry in Directory - should be pointing at a directory entry for a stream */

    assert(CFBF_STGTY_STREAM == file_dir->_mse);

    file_dir->_ulSize = n_bytes; /* patch the _ulSize field of this directory entry */

    file_dir->_cb = 2; /* for the terminating wide-char CH_NULL */

    { /* Write stream name as wide character string in this directory entry, padding with zeros */
    U32 i = 0;
    while(i < sizeof32(file_dir->_ab))
    {
        WCHAR wch = *stream_name;
        if(CH_NULL != wch)
        {
            stream_name++;
            file_dir->_cb += 2;
        }
        writeval_U16_LE(&file_dir->_ab[i], wch);
        i += 2;
    }
    } /*block*/
}

static void
patch_header_mini_stream(
    _Inout_bytecap_c_(0x800) P_StructuredStorageHeader p_structured_storage_header,
    _InVal_     U32 n_bytes)
{
    const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
    P_StructuredStorageDirectoryEntry file_dir;
    P_U32 p_next_sector_number;
    CFBF_SECT sector_number;
    U32 byte_offset;

    /* First FAT sector occupies sector zero (and FAT for sector zero is a FAT marker) */
    assert(0 == p_structured_storage_header->_sectFat[0]);
    p_next_sector_number = PtrAddBytes(P_U32, (p_structured_storage_header + 1), 0 * sector_size);
    assert(CFBF_FATSECT == p_next_sector_number[0]);

    /* Directory occupies sector one (and only sector one, so FAT for sector one is end-of-chain) */
    assert(1 == p_structured_storage_header->_sectDirStart);
    file_dir = PtrAddBytes(P_StructuredStorageDirectoryEntry, (p_structured_storage_header + 1), 1 * sector_size);
    assert(CFBF_ENDOFCHAIN == p_next_sector_number[1]);

    /* First (and only) sector number in the chain for the mini-stream FAT (2) is stored in the header  */
    assert(2 == p_structured_storage_header->_sectMiniFatStart);
    assert(1 == p_structured_storage_header->_csectMiniFat);
    assert(CFBF_ENDOFCHAIN == p_next_sector_number[2]);

    /* First sector number in the chain for the mini-stream (3) is stored in the Root Entry  */
    assert(3 == file_dir->_sectStart);

    file_dir++; /* Skip Root Entry in Directory - should be pointing at entry for our stream */

    { /* The mini-stream FAT occupies sector two */
    P_U32 p_mini_stream_next_sector_number = PtrAddBytes(P_U32, (p_structured_storage_header + 1), 2 * sector_size);
    const U32 mini_stream_sector_size = (U32) 1 << p_structured_storage_header->_uMiniSectorShift;
    U32 mini_stream_sector_number;

    /* first clear the entire mini-stream FAT sector loaded from the template */
    memset32(p_mini_stream_next_sector_number, 0xFF, sector_size); /* CFBF_FREESECT */

    /* our data stream starts at offset zero in the mini-stream */
    mini_stream_sector_number = 0;

    /* loop writing the next mini-stream sector number in the chain - trivial for this contiguous set of mini-stream sectors */
    for(byte_offset = 0; (byte_offset + mini_stream_sector_size) < n_bytes; ++mini_stream_sector_number, byte_offset += mini_stream_sector_size)
        p_mini_stream_next_sector_number[mini_stream_sector_number] = (mini_stream_sector_number + 1);

    /* terminate the mini-stream's chain */
    p_mini_stream_next_sector_number[mini_stream_sector_number] = CFBF_ENDOFCHAIN;
    } /*block*/

    /* The mini-stream itself (containing our stream) starts at sector three */
    sector_number = 3;

    /* loop writing the next sector number in the chain - easy for this contiguous set of sectors */
    for(byte_offset = 0; (byte_offset + sector_size) < n_bytes; ++sector_number, byte_offset += sector_size)
        p_next_sector_number[sector_number] = (sector_number + 1);

    /* terminate the mini-stream's chain */
    p_next_sector_number[sector_number++] = CFBF_ENDOFCHAIN;

    /* finally clear the tail of the FAT sector loaded from the template */
    assert(sector_size >= sector_number * sizeof32(CFBF_SECT));
    if(sector_size >= sector_number * sizeof32(CFBF_SECT))
        memset32(&p_next_sector_number[sector_number], 0xFF, sector_size - sector_number * sizeof32(CFBF_SECT)); /* CFBF_FREESECT */
}

static void
patch_header_standard(
    _Inout_bytecap_c_(0x600) P_StructuredStorageHeader p_structured_storage_header,
    _InVal_     U32 n_bytes)
{
    const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
    P_StructuredStorageDirectoryEntry file_dir;
    P_U32 p_next_sector_number;
    CFBF_SECT sector_number;
    U32 byte_offset;

    /* First FAT sector occupies sector zero (and FAT for sector zero is a FAT marker) */
    assert(0 == p_structured_storage_header->_sectFat[0]);
    p_next_sector_number = PtrAddBytes(P_U32, (p_structured_storage_header + 1), 0 * sector_size);
    assert(CFBF_FATSECT == p_next_sector_number[0]);

    /* Directory occupies sector one (and only sector one, so FAT for sector one is end-of-chain) */
    assert(1 == p_structured_storage_header->_sectDirStart);
    file_dir = PtrAddBytes(P_StructuredStorageDirectoryEntry, (p_structured_storage_header + 1), 1 * sector_size);
    assert(CFBF_ENDOFCHAIN == p_next_sector_number[1]);

    file_dir++; /* Skip Root Entry in Directory - should be pointing at entry for our stream */

    /* Refine code here by calculating the number of sectors the standard file will occupy and
     * therefore how many FAT blocks are needed. Allocate these FAT blocks. Note that the
     * second FAT block will not be contiguous with the first as the directory gets in the way. */

    /* This standard stream starts at sector two */
    sector_number = 2;

    /* First sector number in the chain for this standard stream (2) is stored in the directory entry for our stream */
    assert(file_dir->_sectStart == sector_number);

    /* loop writing the next sector number in the chain - trivial for this contiguous set of sectors */
    for(byte_offset = 0; (byte_offset + sector_size) < n_bytes; ++sector_number, byte_offset += sector_size)
        p_next_sector_number[sector_number] = (sector_number + 1);

    /* terminate this standard stream's chain */
    p_next_sector_number[sector_number] = CFBF_ENDOFCHAIN;

    /* check for overflow of first FAT sector (> 126KB file) */
    assert(sector_number < sector_size / sizeof32(CFBF_SECT));
}

_Check_return_
_Ret_maybenull_
static P_StructuredStorageHeader
cfbf_write_read_template(
    _In_z_      PCTSTR template_leafname,
    _InVal_     U32 header_bytes,
    _OutRef_    P_STATUS p_status)
{
    TCHARZ template_filename[BUF_MAX_PATHSTRING];
    P_StructuredStorageHeader p_structured_storage_header;
    U32 bytes_read;
    FILE_HANDLE file_handle;

    if(file_find_on_path(template_filename, elemof32(template_filename), file_get_search_path(), template_leafname) <= 0)
    {
        *p_status = create_error(ERR_CFBF_SAVE_NEEDS_TEMPLATE);
        return(NULL);
    }

    if(NULL == (p_structured_storage_header = al_ptr_alloc_bytes(P_StructuredStorageHeader, header_bytes, p_status)))
        return(NULL);

    if(status_ok(*p_status = t5_file_open(template_filename, file_open_read, &file_handle, TRUE)))
    {
        *p_status = file_read_bytes(p_structured_storage_header, header_bytes, &bytes_read, file_handle);

        status_accumulate(*p_status, t5_file_close(&file_handle));
    }

    if(status_fail(*p_status))
    {
        al_ptr_free(p_structured_storage_header);
        return(NULL);
    }

    return(p_structured_storage_header);
}

_Check_return_
extern STATUS
cfbf_write_stream_in_storage(
    _InRef_opt_ P_ARRAY_HANDLE p_array_handle_storage,
    _In_opt_z_  PCTSTR storage_filename,
    _InVal_     T5_FILETYPE storage_filetype,
    _In_z_      PCTSTR stream_name,
    _In_reads_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes)
{
    STATUS status;

#if WINDOWS && 0

    if(NULL == storage_filename)
        return(ERR_CFBF_SAVE_NEEDS_FILENAME);

    { /* use Windows' native methods */
    PCWSTR wstr_storage_filename = _wstr_from_tstr(storage_filename);
    PCWSTR wstr_stream_name = _wstr_from_tstr(stream_name);
    status = stg_cfbf_write_stream_in_storage(wstr_storage_filename, wstr_stream_name, p_data, n_bytes);
    } /*block*/

#else

    const BOOL use_mini_stream = (n_bytes < 4096);
    const U32 header_bytes = sizeof32(StructuredStorageHeader) + (use_mini_stream ? 3 : 2) * 512;
    P_StructuredStorageHeader p_structured_storage_header;
    PCTSTR template_leafname =
        use_mini_stream
            ? TEXT("Support") FILE_DIR_SEP_TSTR TEXT("cfbf-mini")
            : TEXT("Support") FILE_DIR_SEP_TSTR TEXT("cfbf");

    /* read in the empty compound file template file appropriate to this size of file */
    if(NULL == (p_structured_storage_header = cfbf_write_read_template(template_leafname, header_bytes, &status)))
        return(status);

    if(status_ok(status))
    {
#if CHECKING
        assert(0xFFFEU == p_structured_storage_header->_uByteOrder); /* Must be Intel Byte Order */
        assert(0x33 <= p_structured_storage_header->_uMinorVersion); /* 0x33 reference, 0x3E in cfbf templates */
        assert(3 == p_structured_storage_header->_uDllVersion);
        assert(512U == (1U << p_structured_storage_header->_uSectorShift));
        assert(1 == p_structured_storage_header->_sectDirStart);
        assert(4096U == p_structured_storage_header->_ulMiniSectorCutoff);
        if(use_mini_stream)
        {
            assert(64U == (1U << p_structured_storage_header->_uMiniSectorShift));
            assert(2 == p_structured_storage_header->_sectMiniFatStart);
            assert(1 == p_structured_storage_header->_csectMiniFat);
        }
        else
        {
            assert(CFBF_ENDOFCHAIN == p_structured_storage_header->_sectMiniFatStart);
            assert(0 == p_structured_storage_header->_csectMiniFat);
        }
#endif

        /* patch the header */
        patch_header_common(p_structured_storage_header, stream_name, n_bytes);

        if(use_mini_stream)
            patch_header_mini_stream(p_structured_storage_header, n_bytes);
        else
            patch_header_standard(p_structured_storage_header, n_bytes);

        assert((NULL != storage_filename) || (NULL != p_array_handle_storage));

        if(NULL == storage_filename)
        {   /* write the desired file to caller's memory object */

            /* pad the whole file up to sector boundary - watch out for unwanted optimisation */
            const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
            const U32 total_sectors = idiv_ceil(header_bytes + n_bytes, sector_size);
            const U32 total_bytes = total_sectors * sector_size;
            P_BYTE p_storage_data = al_array_extend_by_BYTE(p_array_handle_storage, total_bytes, PC_ARRAY_INIT_BLOCK_NONE, &status);

            if(NULL != p_storage_data)
            {
                /* write the header */
                memcpy32(p_storage_data, p_structured_storage_header, header_bytes);

                /* append the data stream */
                memcpy32(p_storage_data + header_bytes, p_data, n_bytes);

                /* pad the whole file up to sector boundary */
                memset32(p_storage_data + header_bytes + n_bytes, 0, total_bytes - (header_bytes + n_bytes));
            }
        }
        else
        {   /* write the desired file */
            FILE_HANDLE file_handle;

            if(status_ok(status = t5_file_open(storage_filename, file_open_write, &file_handle, TRUE)))
            {
#if !RISCOS
                UNREFERENCED_PARAMETER_InVal_(storage_filetype);
#else
                status_assert(file_set_risc_os_filetype(file_handle, storage_filetype));
#endif

                /* write the header */
                status = file_write_bytes(p_structured_storage_header, header_bytes, file_handle);

                /* append the data stream */
                if(status_ok(status))
                    status = file_write_bytes(p_data, n_bytes, file_handle);

                /* pad the whole file up to sector boundary */
                if(status_ok(status))
                    status = file_pad(file_handle, p_structured_storage_header->_uSectorShift);

                status_accumulate(status, t5_file_close(&file_handle));
            }
        }
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_structured_storage_header));

#endif

    return(status);
}

/* end of cfbfwrite.c */
