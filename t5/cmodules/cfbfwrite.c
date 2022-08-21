/* cfbfwrite.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) access functions */

/* Copyright (C) 2014-2021 Stuart Swales */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/cfbf.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

_Check_return_
_Ret_maybenull_
static P_StructuredStorageHeader
cfbf_write_read_template(
    _In_z_      PCTSTR template_leafname,
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status)
{
    TCHARZ template_filename[BUF_MAX_PATHSTRING];
    P_StructuredStorageHeader p_structured_storage_header;
    U32 bytes_read;
    FILE_HANDLE file_handle;

    if(file_find_on_path(template_filename, elemof32(template_filename), file_get_resources_path(), template_leafname) <= 0)
    {
        *p_status = create_error(ERR_CFBF_SAVE_NEEDS_TEMPLATE);
        return(NULL);
    }

    if(NULL == (p_structured_storage_header = al_ptr_alloc_bytes(P_StructuredStorageHeader, n_bytes, p_status)))
        return(NULL);

    if(status_ok(*p_status = t5_file_open(template_filename, file_open_read, &file_handle, TRUE)))
    {
        *p_status = file_read_bytes(p_structured_storage_header, n_bytes, &bytes_read, file_handle);

        status_accumulate(*p_status, t5_file_close(&file_handle));
    }

    if(status_fail(*p_status))
    {
        al_ptr_free(p_structured_storage_header);
        return(NULL);
    }

    return(p_structured_storage_header);
}

static void
patch_header_stream_info(
    _Inout_     P_StructuredStorageDirectoryEntry file_dir,
    _In_z_      PCTSTR stream_name,
    _InVal_     U32 n_bytes)
{
    //assert(CFBF_STGTY_STREAM == file_dir->_mse);
    file_dir->_mse = CFBF_STGTY_STREAM;
    file_dir->_bflags = CFBF_DECOLOR_BLACK;

    file_dir->_ulSize = n_bytes; /* patch the _ulSize field of this directory entry */

    { /* Write stream name as wide character string in this directory entry, padding with zeros */
    U32 i;

    file_dir->_cb = 2; /* for the terminating wide-char CH_NULL */

    for(i = 0; i < sizeof32(file_dir->_ab); i += 2)
    {
        WCHAR wch = *stream_name;
        if(CH_NULL != wch)
        {
            stream_name++;
            file_dir->_cb += 2;
        }
        writeval_U16_LE(&file_dir->_ab[i], wch);
    }
    } /*block*/
}

static void
patch_header_mini_stream( /* one sector of StructuredStorageHeader plus three sectors of FAT, Directory & mini-stream FAT in that order */
    _Inout_bytecap_(header_bytes) P_StructuredStorageHeader p_structured_storage_header,
    _InVal_     U32 header_bytes,
    _In_z_      PCTSTR stream_name,
    _InVal_     U32 n_bytes)
{
    const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
    const P_ANY p_data_sectors = (P_ANY) (p_structured_storage_header + 1); /* data lives in sectors beyond the header */
    P_StructuredStorageDirectoryEntry file_dir;
    P_U32 p_next_sector_number;
    CFBF_SECT sector_number;
    U32 byte_offset;

    UNREFERENCED_PARAMETER_InVal_(header_bytes);

    /* Sector Zero: The First FAT sector (and FAT for this sector is a FAT marker) */
    assert(0 == p_structured_storage_header->_sectFat[0]);
    p_next_sector_number = PtrAddBytes(P_U32, p_data_sectors, p_structured_storage_header->_sectFat[0] * sector_size);
    assert(CFBF_FATSECT == p_next_sector_number[0]);

    /* Sector One: The Directory (and only this sector, so FAT for this sector is end-of-chain) */
    assert(1 == p_structured_storage_header->_sectDirStart);
    file_dir = PtrAddBytes(P_StructuredStorageDirectoryEntry, p_data_sectors, p_structured_storage_header->_sectDirStart * sector_size);
    assert(CFBF_ENDOFCHAIN == p_next_sector_number[1]);
    assert(CFBF_STGTY_ROOT == file_dir->_mse);

    /* First (and only) sector number in the chain for the mini-stream FAT (2) is stored in the header  */
    assert(2 == p_structured_storage_header->_sectMiniFatStart);
    assert(1 == p_structured_storage_header->_csectMiniFat);
    assert(CFBF_ENDOFCHAIN == p_next_sector_number[2]);

    /* First sector number in the chain for the mini-stream (3) is stored in the Root Entry (first Directory entry) */
    assert(3 == file_dir->_sectStart);

    file_dir++; /* Skip Root Entry in Directory - should be pointing at entry for our stream */

    { /* Sector Two: The mini-stream FAT */
    P_U32 p_mini_stream_next_sector_number = PtrAddBytes(P_U32, p_data_sectors, 2 * sector_size);
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

    /* Sector Three: The mini-stream itself (containing our stream) starts at sector three */
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

    patch_header_stream_info(file_dir, stream_name, n_bytes);
}

static void
patch_header_standard( /* one sector of StructuredStorageHeader plus two sectors of Directory & FAT */
    _Inout_bytecap_(header_bytes) P_StructuredStorageHeader p_structured_storage_header,
    _InVal_     U32 header_bytes,
    _In_z_      PCTSTR stream_name,
    _InVal_     U32 n_bytes)
{
    const U32 sector_size = (U32) 1 << p_structured_storage_header->_uSectorShift;
    const U32 FAT_entries_per_sector = sector_size / sizeof32(CFBF_SECT);
    const P_ANY p_data_sectors = PtrAddBytes(P_ANY, p_structured_storage_header, sector_size); /* data lives in sectors beyond the sector containing the header */
    P_StructuredStorageDirectoryEntry file_dir;
    P_U32 p_next_sector_number;
    CFBF_SECT sector_number;
    U32 n_FAT_entries_required;
    U32 byte_offset;

    UNREFERENCED_PARAMETER_InVal_(header_bytes);

    /* Sector Zero: The Directory (and only this sector, so FAT for this sector is end-of-chain) */
    assert(0 == p_structured_storage_header->_sectDirStart);
    file_dir = PtrAddBytes(P_StructuredStorageDirectoryEntry, p_data_sectors, p_structured_storage_header->_sectDirStart * sector_size);
    assert(CFBF_STGTY_ROOT == file_dir->_mse);
    file_dir->_sidChild = 1; /* Stream ID of child (i.e. this stream's directory entry) */

    /* Sector One: The First FAT sector (and FAT for this sector is a FAT marker) */
    assert(1 == p_structured_storage_header->_sectFat[0]);
    p_next_sector_number = PtrAddBytes(P_U32, p_data_sectors, p_structured_storage_header->_sectFat[0] * sector_size);

    assert(CFBF_ENDOFCHAIN == p_next_sector_number[p_structured_storage_header->_sectDirStart]);
    assert(CFBF_FATSECT    == p_next_sector_number[p_structured_storage_header->_sectFat[0]]);

    file_dir++; /* Skip Root Entry in Directory - should be pointing at entry for our stream */

    /* Refine code here by calculating the number of sectors the standard file will occupy and
     * therefore how many FAT blocks are needed. Allocate these FAT blocks. */
    n_FAT_entries_required = (n_bytes / sector_size) + 1 /*CFBF_ENDOFCHAIN*/;
    assert(n_FAT_entries_required <= FAT_entries_per_sector);

    /* Sector Two: This standard stream starts at sector two */
    sector_number = 2;

    /* First sector number in the chain for this standard stream (2) is stored in the directory entry for our stream */
    //assert(file_dir->_sectStart == sector_number);
    file_dir->_sectStart = sector_number;

    /* loop writing the next sector number in the chain - trivial for this contiguous set of sectors */
    for(byte_offset = 0; (byte_offset + sector_size) < n_bytes; ++sector_number, byte_offset += sector_size)
        p_next_sector_number[sector_number] = (sector_number + 1);

    /* terminate this standard stream's chain */
    p_next_sector_number[sector_number] = CFBF_ENDOFCHAIN;

    /* check for overflow of first FAT sector (> 63Kb file) */
    assert(sector_number < FAT_entries_per_sector);

    patch_header_stream_info(file_dir, stream_name, n_bytes);
}

/* if it can't be wrapped in structured storage, dump the data out raw - many readers will cope */

_Check_return_
static STATUS
cfbf_write_stream_raw(
    _InRef_opt_ P_ARRAY_HANDLE p_array_handle_storage,
    _In_opt_z_  PCTSTR storage_filename,
    _InVal_     T5_FILETYPE storage_filetype,
    _In_z_      PCTSTR stream_name,
    _In_reads_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes)
{
    STATUS status;

    assert((NULL != storage_filename) || (NULL != p_array_handle_storage));

    if(NULL == storage_filename)
    {   /* write the desired file to caller's memory object (raw) */
        const U32 total_bytes = n_bytes;
        P_BYTE p_raw_data = al_array_extend_by_BYTE(p_array_handle_storage, total_bytes, PC_ARRAY_INIT_BLOCK_NONE, &status);

        if(NULL != p_raw_data)
        {   /* write the data stream */
            memcpy32(p_raw_data, p_data, n_bytes);
        }
    }
    else
    {   /* write the desired file (raw) */
        FILE_HANDLE file_handle;

        if(status_ok(status = t5_file_open(storage_filename, file_open_write, &file_handle, TRUE)))
        {
#if !RISCOS
            UNREFERENCED_PARAMETER_InVal_(storage_filetype);
#else
            status_assert(file_set_risc_os_filetype(file_handle, storage_filetype));
#endif

            UNREFERENCED_PARAMETER_InVal_(stream_name);

            /* write the data stream */
            if(status_ok(status))
                status = file_write_bytes(p_data, n_bytes, file_handle);

            status_accumulate(status, t5_file_close(&file_handle));
        }
    }

    return(status);
}

_Check_return_
static STATUS
cfbf_write_stream_in_storage_homebrew(
    _InRef_opt_ P_ARRAY_HANDLE p_array_handle_storage,
    _In_opt_z_  PCTSTR storage_filename,
    _InVal_     T5_FILETYPE storage_filetype,
    _In_z_      PCTSTR stream_name,
    _In_reads_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes)
{
    STATUS status;
    const BOOL use_mini_stream = (n_bytes < 4096);
    const BOOL use_large_sectors = (n_bytes >= (128-2)*512); /* 63Kb: 128 FAT entries per 512-byte sector, minus two for Directory & FAT */
    const U32 sector_size = (use_large_sectors ? 4096U : 512U);
    const U32 header_bytes = (1U + (use_mini_stream ? 3U : 2U)) * sector_size;
    P_StructuredStorageHeader p_structured_storage_header;
    PCTSTR template_leafname =
        use_mini_stream
            ? TEXT("Support") FILE_DIR_SEP_TSTR TEXT("cfbf-mini")
            :
        use_large_sectors
            ? TEXT("Support") FILE_DIR_SEP_TSTR TEXT("cfbf-4096")
            : TEXT("Support") FILE_DIR_SEP_TSTR TEXT("cfbf");

    /* read in the empty compound file template file appropriate to this size of file */
    if(NULL == (p_structured_storage_header = cfbf_write_read_template(template_leafname, header_bytes, &status)))
        return(status);

    if(status_ok(status))
    {
#if CHECKING
        assert(0xFFFEU == p_structured_storage_header->_uByteOrder); /* Must be Intel Byte Order */
        assert(0x33 <= p_structured_storage_header->_uMinorVersion); /* 0x33 reference, 0x3E in cfbf templates */
        assert((3 == p_structured_storage_header->_uDllVersion) || (4 == p_structured_storage_header->_uDllVersion));
        assert(sector_size == (1U << p_structured_storage_header->_uSectorShift));
        assert(4096U == p_structured_storage_header->_ulMiniSectorCutoff);
        if(use_mini_stream)
        {
            assert(64U == (1U << p_structured_storage_header->_uMiniSectorShift));
            assert(2 == p_structured_storage_header->_sectMiniFatStart);
            assert(1 == p_structured_storage_header->_csectMiniFat);
            assert(1 == p_structured_storage_header->_sectDirStart); /* CFBF mini template has FAT before Directory */
        }
        else
        {
            assert(CFBF_ENDOFCHAIN == p_structured_storage_header->_sectMiniFatStart);
            assert(0 == p_structured_storage_header->_csectMiniFat);
            assert(0 == p_structured_storage_header->_sectDirStart); /* CFBF template now has Directory sector before FAT sectors */
            assert(1 == p_structured_storage_header->_sectFat[0]);
        }
#endif

        /* patch the header */
        if(use_mini_stream)
            patch_header_mini_stream(p_structured_storage_header, header_bytes, stream_name, n_bytes);
        else
            patch_header_standard(p_structured_storage_header, header_bytes, stream_name, n_bytes);

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

                /* append the allocation table */

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

                /* append the allocation table */

                /* append the data stream */
                if(status_ok(status))
                    status = file_write_bytes(p_data, n_bytes, file_handle);

                /* pad the whole file up to sector boundary */
                if(status_ok(status))
                    status = file_pad(file_handle, p_structured_storage_header->_uSectorShift, CH_NULL);

                status_accumulate(status, t5_file_close(&file_handle));
            }
        }
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_structured_storage_header));

    return(status);
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
#if WINDOWS
    static bool use_windows_structured_storage_api = RELEASED || false; /* set to false to debug homebrew access method */

    if( (NULL != storage_filename) && use_windows_structured_storage_api )
    {
        if(NULL == storage_filename)
            return(ERR_CFBF_SAVE_NEEDS_FILENAME);

        { /* use Windows' native structured storage API */
        const PCWSTR wstr_storage_filename = _wstr_from_tstr(storage_filename);
        const PCWSTR wstr_stream_name = _wstr_from_tstr(stream_name);
        return(stg_cfbf_write_stream_in_storage(wstr_storage_filename, wstr_stream_name, p_data, n_bytes));
        } /*block*/
    }
#endif

    /* file too large for current homebrew implementation? */
    if(n_bytes >= (1024-2)*4096) /* 3.992Mb: 1024 FAT entries per 4096-byte sector, minus two for Directory & FAT */
        return(cfbf_write_stream_raw(p_array_handle_storage, storage_filename, storage_filetype, stream_name, p_data, n_bytes));

    return(cfbf_write_stream_in_storage_homebrew(p_array_handle_storage, storage_filename, storage_filetype, stream_name, p_data, n_bytes));
}

/* end of cfbfwrite.c */
