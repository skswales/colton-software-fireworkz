/* cfbf.c */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) access functions */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/cfbf.h"

_Check_return_
static STATUS
compound_file_read_file_header(
    _InoutRef_  P_COMPOUND_FILE p_compound_file);

_Check_return_
static STATUS
compound_file_read_file_MSAT(
    _InoutRef_  P_COMPOUND_FILE p_compound_file);

_Check_return_
static STATUS
compound_file_create_directory_list(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int first_sector_id);

_Check_return_
static STATUS
compound_file_create_common(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    p_compound_file->current_SAT_block = -1;

    p_compound_file->current_SSAT_block = -1;

    p_compound_file->ministream_chain_first_sector_id = CFBF_ENDOFCHAIN;

    status_return(compound_file_read_file_header(p_compound_file));

    status_return(compound_file_read_file_MSAT(p_compound_file));

    return(compound_file_create_directory_list(p_compound_file, p_compound_file->hdr._sectDirStart));
}

_Check_return_
_Ret_maybenull_
extern P_COMPOUND_FILE
compound_file_create_from_data(
    _In_reads_(data_size) PC_BYTE p_data,
    _InVal_     U32 data_size,
    _OutRef_    P_STATUS p_status)
{
    P_COMPOUND_FILE p_compound_file = al_ptr_calloc_elem(COMPOUND_FILE, 1, p_status);

    if(NULL != p_compound_file)
    {
        p_compound_file->file_handle = NULL;
        p_compound_file->p_data = p_data;
        p_compound_file->data_size = data_size;

        *p_status = compound_file_create_common(p_compound_file);

        if(status_fail(*p_status))
            compound_file_dispose(&p_compound_file);
    }

    return(p_compound_file);
}

_Check_return_
_Ret_maybenull_
extern P_COMPOUND_FILE
compound_file_create_from_file_handle(
    _InRef_     FILE_HANDLE file_handle,
    _OutRef_    P_STATUS p_status)
{
    P_COMPOUND_FILE p_compound_file = al_ptr_calloc_elem(COMPOUND_FILE, 1, p_status);

    if(NULL != p_compound_file)
    {
        p_compound_file->file_handle = file_handle;
        p_compound_file->p_data = P_DATA_NONE;
        p_compound_file->data_size = 0;

        *p_status = compound_file_create_common(p_compound_file);

        if(status_fail(*p_status))
            compound_file_dispose(&p_compound_file);
    }

    return(p_compound_file);
}

extern void
compound_file_dispose(
    _InoutRef_  P_P_COMPOUND_FILE p_p_compound_file)
{
    P_COMPOUND_FILE p_compound_file = *p_p_compound_file;

    if(NULL != p_compound_file)
    {
        *p_p_compound_file = NULL;

        if(NULL != p_compound_file->p_standard_sector_buffer)
            al_ptr_free(p_compound_file->p_standard_sector_buffer);

        if(NULL != p_compound_file->full_MSAT)
            al_ptr_free(p_compound_file->full_MSAT);

        if(NULL != p_compound_file->decoded_directory_list)
            al_ptr_free(p_compound_file->decoded_directory_list);

        al_ptr_free(p_compound_file);
    }
}

_Check_return_
extern BOOL
compound_file_file_header_id_test(
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes)
{
    static const BYTE buffer_compound_file_file_id[CFBF_FILE_HEADER_ID_BYTES] =
        { '\xD0', '\xCF', '\x11', '\xE0', '\xA1', '\xB1', '\x1A', '\xE1' };

    if(n_bytes < CFBF_FILE_HEADER_ID_BYTES)
        return(FALSE);

    return(0 == memcmp32(p_data, buffer_compound_file_file_id, CFBF_FILE_HEADER_ID_BYTES));
}

_Check_return_
static STATUS
compound_file_read_file_header(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    STATUS status;
    StructuredStorageHeader * hdr = &p_compound_file->hdr;

    if(NULL != p_compound_file->file_handle)
    {
        status_return(file_rewind(p_compound_file->file_handle));

        status = file_read_bytes_requested(hdr, sizeof32(*hdr), p_compound_file->file_handle);
    }
    else if(0 != p_compound_file->data_size)
    {
        if(0 + sizeof32(*hdr) <= p_compound_file->data_size)
        {
            memcpy32(hdr, p_compound_file->p_data + 0, sizeof32(*hdr));
            status = STATUS_OK;
        }
        else
        {
            assert(0 + sizeof32(*hdr) <= p_compound_file->data_size);
            status = status_check();
        }
    }
    else
        status = status_check();

    status_return(status);

#if TRACE_ALLOWED && 1
    { /* dump this StructuredStorageHeader */
#if 1
    const U32 limit = offsetof32(StructuredStorageHeader, _sectFat); /* _sectFat is dumped just afterwards */
#else
    const U32 limit = sizeof32(StructuredStorageHeader);
#endif
    U32 i;
    trace_0(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader:"));
    /*offsetof32(StructuredStorageHeader,_sectFat)*/
    for(i = 0; i < limit/sizeof32(int); i++)
    {
        if(0 == (i % 4))
            trace_1(TRACE_MODULE_CFBF, TEXT("  offset 0x%.2X |"), i * sizeof32(U32));
        trace_1(TRACE_MODULE_CFBF, TEXT("|%.8X |"), ((PC_U32) hdr)[i]);
    }
    } /*block*/
#endif

    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uMinorVersion=" U16_TFMT), hdr->_uMinorVersion);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uDllVersion=" U16_TFMT), hdr->_uDllVersion);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uByteOrder=" U16_XTFMT), hdr->_uByteOrder);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _csectFat=" U32_TFMT), hdr->_csectFat);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _sectDirStart=" U32_TFMT), hdr->_sectDirStart);

    p_compound_file->standard_sector_size = (U32) 1 << hdr->_uSectorShift;
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: standard_sector_size=" U32_TFMT), p_compound_file->standard_sector_size);

    p_compound_file->ministream_sector_size = (U32) 1 << hdr->_uMiniSectorShift;
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: ministream_sector_size=" U32_TFMT), p_compound_file->ministream_sector_size);

    assert(p_compound_file->standard_sector_size <= COMPOUND_FILE_MAX_SECTOR_SIZE); /* for fixed block arrays at the moment */

    if(hdr->_uSectorShift > (9+3))
    {   /* failed plausibility check (_uSectorShift typically 9) */
        return(status_check());
    }

    if(hdr->_uMiniSectorShift > 6)
    {   /* failed plausibility check (_uMiniSectorShift typically 6) */
        assert(hdr->_uMiniSectorShift == 6);
        return(status_check());
    }

    if(NULL == (p_compound_file->p_standard_sector_buffer = al_ptr_alloc_bytes(P_BYTE, p_compound_file->standard_sector_size, &status)))
        return(status);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
compound_file_read_file_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT sector_id,
    P_ANY dest,
    _OutRef_    P_U32 p_bytesread)
{
    const U32 virtual_filepos = sector_id * p_compound_file->standard_sector_size;
    const U32 actual_filepos = sizeof32(StructuredStorageHeader) + virtual_filepos;
    const U32 bytes_to_read = p_compound_file->standard_sector_size;
    U32 bytes_read = 0;

    trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_file_sector(sector_id %u, filepos ") U32_XTFMT TEXT(")"), sector_id, actual_filepos);
    assert((S32) sector_id >= 0);

    if(NULL != p_compound_file->file_handle)
    {
        STATUS status;

        status = file_seek(p_compound_file->file_handle, actual_filepos, NULL /*hi*/, SEEK_SET);

        if(status_ok(status))
            status = file_read_bytes(dest, bytes_to_read, &bytes_read, p_compound_file->file_handle);

        if(status_fail(status))
            bytes_read = 0; /* in case file_seek() etc. fails */

        *p_bytesread = bytes_read;
        return(status);
    }

    if(0 != p_compound_file->data_size)
    {
        if(actual_filepos <= p_compound_file->data_size) /* starting read within the data? */
        {
            U32 end_filepos = actual_filepos + bytes_to_read; /* oh for access to CPU flags... */

            if((end_filepos >= actual_filepos) && (end_filepos >= bytes_to_read)) /* check overflow */
            {
                if(end_filepos <= p_compound_file->data_size) /* ending read within the data? */
                    bytes_read = bytes_to_read; /* can satisfy whole request */
                else
                {
                    bytes_read = (p_compound_file->data_size - actual_filepos); /* limit request to what we have */
                    trace_3(TRACE_MODULE_CFBF, TEXT("compound_file_read_file_sector(sector_id %u, filepos ") U32_XTFMT TEXT(") - *** limit to " U32_XTFMT), sector_id, actual_filepos, bytes_read);
                }

                *p_bytesread = bytes_read;
                memcpy32(dest, p_compound_file->p_data + actual_filepos, bytes_read);
                return(STATUS_OK);
            }
        }
        assert(actual_filepos <= p_compound_file->data_size);
        assert(actual_filepos + bytes_to_read <= p_compound_file->data_size);
    }

    *p_bytesread = 0;
    return(status_check());
}

_Check_return_
extern STATUS
compound_file_read_SSAT_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int SSAT_sector_id,
    P_ANY dest)
{
    U32 ministream_filepos = (U32) SSAT_sector_id * p_compound_file->ministream_sector_size;
    U32 this_ministream_filepos = 0;
    int ministream_chain_sector_id;

    trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_SSAT_sector(SSAT_sector_id %d): short_stream_filepos ") U32_XTFMT, SSAT_sector_id, ministream_filepos);
    assert(SSAT_sector_id >= 0);

    for(ministream_chain_sector_id = p_compound_file->ministream_chain_first_sector_id;
        ministream_chain_sector_id >= 0;
        ministream_chain_sector_id = compound_file_get_next_sector_id(p_compound_file, ministream_chain_sector_id))
    {
        if(ministream_filepos < (this_ministream_filepos + p_compound_file->standard_sector_size))
        {
            U32 bytes_read;

            trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_SSAT_sector(SSAT_sector_id %d): this_short_stream_filepos ") U32_XTFMT, SSAT_sector_id, this_ministream_filepos);
            status_return(compound_file_read_file_sector(p_compound_file, ministream_chain_sector_id, p_compound_file->p_standard_sector_buffer, &bytes_read));

            memcpy32(dest, p_compound_file->p_standard_sector_buffer + (ministream_filepos - this_ministream_filepos), p_compound_file->ministream_sector_size);

            return(STATUS_DONE);
        }

        this_ministream_filepos += p_compound_file->standard_sector_size;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
compound_file_read_file_MSAT(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    STATUS status;
    U32 msat_entry_bytes;
    CFBF_SECT sector_id;

    if(NULL == (p_compound_file->full_MSAT =
                al_ptr_calloc_bytes(CFBF_SECT *, sizeof32(p_compound_file->hdr._sectFat) + (p_compound_file->standard_sector_size * (U32) p_compound_file->hdr._csectDif), &status)))
        return(status);

    /* load the first 109 MSAT entries from the file header */
    msat_entry_bytes = sizeof32(p_compound_file->hdr._sectFat);
    memcpy32(p_compound_file->full_MSAT, p_compound_file->hdr._sectFat, msat_entry_bytes);

#if TRACE_ALLOWED
    {
    const CFBF_SECT * x = p_compound_file->full_MSAT;
    const U32 msat_entries = msat_entry_bytes/sizeof32(*x);
    U32 list_index;
    BOOL needs_newline = TRUE;
    PTR_ASSERT(x);
    trace_1(TRACE_MODULE_CFBF, TEXT("MSAT from file header (%u entries):|"), msat_entries);
    for(list_index = 0; list_index < msat_entries; list_index++, x++)
    {
        if(0 == (list_index % 8))
        {
            trace_1(TRACE_MODULE_CFBF, TEXT("  entry %.3u |"), list_index);
            needs_newline = FALSE;
        }
        else
            needs_newline = TRUE;
        trace_1(TRACE_MODULE_CFBF, TEXT("|%.8X |"), *x);
    }
    if(needs_newline)
        trace_0(TRACE_MODULE_CFBF, TEXT(""));
    } /*block*/
#endif

    /* load the rest of the MSAT entries from the file */
    sector_id = p_compound_file->hdr._sectDifStart;

    if(CFBF_ENDOFCHAIN != sector_id)
    {
        P_BYTE p = PtrAddBytes(P_BYTE, p_compound_file->full_MSAT, msat_entry_bytes);
        CFBF_FSINDEX i;

        for(i = 0; i < p_compound_file->hdr._csectDif; i++)
        {
            U32 bytes_read;

            assert((S32) sector_id >= 0);
            status_assert(compound_file_read_file_sector(p_compound_file, sector_id, p_compound_file->p_standard_sector_buffer, &bytes_read));
            assert(bytes_read == p_compound_file->standard_sector_size);

            msat_entry_bytes = p_compound_file->standard_sector_size - sizeof32(int); /* don't copy the link word */
            memcpy32(p, p_compound_file->p_standard_sector_buffer, msat_entry_bytes);

#if TRACE_ALLOWED
            {
            const CFBF_SECT * x = (const CFBF_SECT *) p;
            const U32 msat_entries = msat_entry_bytes/sizeof32(*x);
            U32 list_index;
            BOOL needs_newline = TRUE;
            trace_2(TRACE_MODULE_CFBF, TEXT("MSAT from file DIF (part %d, %u entries):"), i, msat_entries);
            for(list_index = 0; list_index < msat_entries; list_index++, x++)
            {
                if(0 == (list_index % 8))
                {
                    trace_2(TRACE_MODULE_CFBF, TEXT("DIF %d entry %.3u |"), i, list_index);
                    needs_newline = FALSE;
                }
                else
                    needs_newline = TRUE;
                trace_1(TRACE_MODULE_CFBF, TEXT("|%.8X |"), *x);
            }
            if(needs_newline)
                trace_0(TRACE_MODULE_CFBF, TEXT(""));
            } /*block*/
#endif

            p += msat_entry_bytes;

            sector_id = * (const CFBF_SECT *) (p_compound_file->p_standard_sector_buffer + msat_entry_bytes); /* chain using the link word */
        }

        assert(CFBF_ENDOFCHAIN == sector_id);
    }

    return(status);
}

static void
compound_file_ensure_SAT_block(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int sector_id)
{
    U32 sat_index = (U32) sector_id / (p_compound_file->standard_sector_size/sizeof32(int));
    int SAT_block = p_compound_file->full_MSAT[sat_index];

    if(p_compound_file->current_SAT_block != SAT_block)
    {   /* not currently cached, so must reset and load */
        U32 bytes_read;
        p_compound_file->current_SAT_block = -1;

        if(status_ok(compound_file_read_file_sector(p_compound_file, SAT_block, p_compound_file->current_SAT_block_list, &bytes_read)))
        {
            assert(bytes_read == p_compound_file->standard_sector_size);
            p_compound_file->current_SAT_block = SAT_block;
        }
    }
}

static void
compound_file_ensure_SSAT_block(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int SSAT_sector_id)
{
    U32 ssat_index = (U32) SSAT_sector_id / (p_compound_file->standard_sector_size/sizeof32(int)); /* NB SSAT is stored in standard sectors */
    int SSAT_block = p_compound_file->hdr._sectMiniFatStart;

    consume(U32, ssat_index);
    assert(ssat_index == 0);
    assert(p_compound_file->hdr._csectMiniFat == 1);

    if(p_compound_file->current_SSAT_block != SSAT_block)
    {   /* not currently cached, so must reset and load */
        U32 bytes_read;

        p_compound_file->current_SSAT_block = -1;

        if(status_ok(compound_file_read_file_sector(p_compound_file, SSAT_block, p_compound_file->current_SSAT_block_list, &bytes_read)))
        {
            assert(bytes_read == p_compound_file->standard_sector_size);
            p_compound_file->current_SSAT_block = SSAT_block;
        }
    }
}

/*
get the next sector_id in this object's chain
*/

_Check_return_
extern int /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int sector_id)
{
    /* ensure that the block list containing this sector_id is loaded */
    compound_file_ensure_SAT_block(p_compound_file, sector_id);

    {
    int SAT_block_list_index = sector_id % (p_compound_file->standard_sector_size/sizeof32(int));
    int next_sector_id = p_compound_file->current_SAT_block_list[SAT_block_list_index];
    trace_2(TRACE_MODULE_CFBF, TEXT("sector_id %u -> next sector_id %u"), sector_id, next_sector_id);
    return(next_sector_id);
    } /*block*/
}

_Check_return_
extern int /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_SSAT_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int SSAT_sector_id)
{
    /* ensure that the block list containing this sector_id is loaded */
    compound_file_ensure_SSAT_block(p_compound_file, SSAT_sector_id);

    {
    int SSAT_block_list_index = SSAT_sector_id % (p_compound_file->standard_sector_size/sizeof32(int)); /* NB SSAT is stored in standard sectors */
    int next_SSAT_sector_id = p_compound_file->current_SSAT_block_list[SSAT_block_list_index];
    trace_2(TRACE_MODULE_CFBF, TEXT("SSAT_sector_id %d -> next SSAT_sector_id %d"), SSAT_sector_id, next_SSAT_sector_id);
    return(next_SSAT_sector_id);
    } /*block*/
}

#if TRACE_ALLOWED

static void
compound_file_dump_directory_entry(
    _InRef_     P_COMPOUND_FILE p_compound_file,
    _InRef_     PC_StructuredStorageDirectoryEntry file_dir,
    _In_        int dirnum)
{
    IGNOREPARM_InRef_(p_compound_file);

    if(CFBF_STGTY_INVALID == file_dir->_mse)
        return;

    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageDirectoryEntry #%d |"), dirnum);

    { /* dump this SSDE */
    int i;
    for(i = 0; i < sizeof32(StructuredStorageDirectoryEntry)/sizeof32(int); i++)
    {
        if(0 == (i % 4))
            trace_1(TRACE_MODULE_CFBF, TEXT("  offset 0x%.2X |"), i * sizeof32(int));
        trace_1(TRACE_MODULE_CFBF, TEXT("|%.8X |"), ((const int *) file_dir)[i]);
    }
    } /*block*/

    trace_2(TRACE_MODULE_CFBF, TEXT("  _mse=%d(%s) |"),
        file_dir->_mse,
        (CFBF_STGTY_ROOT == file_dir->_mse)
        ? TEXT("ROOT")
        : (CFBF_STGTY_STORAGE == file_dir->_mse)
        ? TEXT("STORAGE")
        : (CFBF_STGTY_STREAM == file_dir->_mse)
        ? TEXT("STREAM")
        : TEXT("OTHER"));

    { /* decode wide character name */
    int i = 0;

#if RISCOS

    PC_BYTE p = file_dir->_ab;

    if(*p < CH_SPACE)
    {
        U16 unknown = * ((PC_U16) p);
        trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), unknown);
        p += 2; /* step over unknown short */
        i += 2;
    }
    for( ; i < file_dir->_cb/2; i++, p += 2)
    {
        if(CH_NULL == p[1])
            trace_1(TRACE_MODULE_CFBF, TEXT("|%c|"), p[0]);
        else
            trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), * ((PC_U16) p));
    }

#elif WINDOWS

    const WCHAR * p = (const WCHAR *) file_dir->_ab;

    if((* (PC_BYTE) p) < CH_SPACE)
    {
        U16 unknown = * ((PC_U16) p);
        trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), unknown);
        p += 1; /* step over unknown short */
        i += 2;
    }

    __pragma(warning(suppress: 28182))
    for( ; i < file_dir->_cb/2; i++, p++)
    {
        trace_1(TRACE_MODULE_CFBF, TEXT("|%lc|"), *p);
    }

#endif /* OS */
    } /*block*/

    trace_1(TRACE_MODULE_CFBF, TEXT("  _sidLeftSib  = ") U32_XTFMT, file_dir->_sidLeftSib);
    trace_1(TRACE_MODULE_CFBF, TEXT("  _sidRightSib = ") U32_XTFMT, file_dir->_sidRightSib);
    trace_1(TRACE_MODULE_CFBF, TEXT("  _sidChild    = ") U32_XTFMT, file_dir->_sidChild);

    if(CFBF_STGTY_STREAM != file_dir->_mse)
    {
#if WINDOWS
        FILETIME filetime;
        filetime.dwLowDateTime  = file_dir->_time_0_dwLowDateTime;
        filetime.dwHighDateTime = file_dir->_time_0_dwHighDateTime;
        if((filetime.dwLowDateTime == 0) && (filetime.dwHighDateTime == 0))
            trace_0(TRACE_MODULE_CFBF, TEXT("  _time[0]: NULL"));
        else
        {
            SYSTEMTIME systemtime;
            FileTimeToSystemTime(&filetime, &systemtime);
            trace_7(TRACE_MODULE_CFBF, TEXT("  _time[0]: %.4u-%.2u-%.2u %.2u:%.2u:%.2u.%.3u"),
                    systemtime.wYear, systemtime.wMonth, systemtime.wDay,
                    systemtime.wHour, systemtime.wMinute, systemtime.wSecond, systemtime.wMilliseconds);
        }

        filetime.dwLowDateTime  = file_dir->_time_1_dwLowDateTime;
        filetime.dwHighDateTime = file_dir->_time_1_dwHighDateTime;
        if((filetime.dwLowDateTime == 0) && (filetime.dwHighDateTime == 0))
            trace_0(TRACE_MODULE_CFBF, TEXT("  _time[1]: NULL"));
        else
        {
            SYSTEMTIME systemtime;
            FileTimeToSystemTime(&filetime, &systemtime);
            trace_7(TRACE_MODULE_CFBF, TEXT("  _time[1]: %.4u-%.2u-%.2u %.2u:%.2u:%.2u.%.3u"),
                    systemtime.wYear, systemtime.wMonth, systemtime.wDay,
                    systemtime.wHour, systemtime.wMinute, systemtime.wSecond, systemtime.wMilliseconds);
        }
#else
#endif /* OS */
    }

    trace_1(TRACE_MODULE_CFBF, TEXT("  _sectStart = ") U32_XTFMT, file_dir->_sectStart);
    trace_1(TRACE_MODULE_CFBF, TEXT("  _ulSize = ") U32_XTFMT, file_dir->_ulSize);
}

#endif /* TRACE_ALLOWED */

static void
compound_file_decode_info_from_file_dir(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InRef_     PC_StructuredStorageDirectoryEntry file_dir)
{
    COMPOUND_FILE_DECODED_DIRECTORY * decoded_directory;

#if TRACE_ALLOWED
    compound_file_dump_directory_entry(p_compound_file, file_dir, p_compound_file->decoded_directory_count);
#endif

    decoded_directory = &p_compound_file->decoded_directory_list[p_compound_file->decoded_directory_count];

#if WINDOWS || RISCOS

    { /* Transfer the name as Unicode string */
    const WCHAR * p = (const WCHAR *) file_dir->_ab;
    WCHAR * q = decoded_directory->name.wchar;
    int j;
    if((* (const char *) p) < CH_SPACE)
        p += 1; /* skip leading short */

    __pragma(warning(suppress: 28182))
    for(j = 0; j < file_dir->_cb/2; j++, p++)
    {
        *q++ = *p;
    }
    *q = CH_NULL;

    trace_1(TRACE_MODULE_CFBF, TEXT("Decoded name %ls"), decoded_directory->name.wchar);
    } /*block*/

#else

    { /* force the name into a BYTE string */
    PC_BYTE p = file_dir->_ab;
    char * q = decoded_directory->name.u8;
    int j;
    if(*p < CH_SPACE)
        p += 2; /* skip leading short */
    for(j = 0; j < file_dir->_cb/2; j++, p += 2)
    {
        if(CH_NULL == p[1]) /* skip all wide chars that don't have top-byte-zero */
            *q++ = *p;
        else
            *q++ = CH_UNDERSCORE;
    }
    *q = CH_NULL;

    trace_1(TRACE_MODULE_CFBF, TEXT("Decoded name %s"), decoded_directory->name.u8);
    } /*block*/

#endif /* OS */

    /* copy some members directly from the StructuredStorageDirectoryEntry */
    decoded_directory->_mse = file_dir->_mse;
    decoded_directory->_sectStart = file_dir->_sectStart;
    decoded_directory->_ulSize = file_dir->_ulSize;

    p_compound_file->decoded_directory_count++;
}

#if TRACE_ALLOWED

static void
compound_file_dump_decoded_directory_list(
    _InRef_     P_COMPOUND_FILE p_compound_file)
{
    const COMPOUND_FILE_DECODED_DIRECTORY * decoded_directory = p_compound_file->decoded_directory_list;
    U32 directory_index;

    trace_0(TRACE_MODULE_CFBF, TEXT("Decoded Directory List:"));

    for(directory_index = 0;
        directory_index < p_compound_file->decoded_directory_count;
        directory_index++, decoded_directory++)
    {
        int offset = 0;
        U8Z buffer[256];

#if WINDOWS || RISCOS
        offset += xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("%ls"), decoded_directory->name.wchar);
#else
        offset += xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("%s"), decoded_directory->name.u8);
#endif /* OS */

        memset32(buffer + offset, CH_SPACE, sizeof32(buffer) - offset); /* replace CH_NULL and other garbage Windows CRT has dumoped in there */

        offset = 60; /* print the object size at fixed offset */

        if(CFBF_STGTY_STREAM == decoded_directory->_mse)
            consume_int(xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("STREAM ")));
        else if(CFBF_STGTY_STORAGE == decoded_directory->_mse)
            consume_int(xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("STORAGE ")));

        consume_int(xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("%8d %s"),
                              decoded_directory->_ulSize,
                              (decoded_directory->_ulSize < p_compound_file->hdr._ulMiniSectorCutoff) ? "SSAT" : "SAT"));

        trace_0(TRACE_MODULE_CFBF, report_sbstr(buffer));
    }
}

#endif /* TRACE_ALLOWED */

_Check_return_
static STATUS
compound_file_create_directory_list(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _In_        int first_sector_id)
{
    STATUS status;
    P_StructuredStorageDirectoryEntry file_dirs = (P_StructuredStorageDirectoryEntry) p_compound_file->p_standard_sector_buffer; /* always read via this sector-size buffer*/
    const U32 dirs_per_sector = p_compound_file->standard_sector_size / sizeof32(StructuredStorageDirectoryEntry);
    U32 n_directory_entries = 0;
    int sector_id;
    U32 i;

    /* first get the size of the entries */
    for(sector_id = first_sector_id;
        sector_id >= 0;
        sector_id = compound_file_get_next_sector_id(p_compound_file, sector_id))
    {
        U32 bytes_read;

        status_assert(compound_file_read_file_sector(p_compound_file, sector_id, file_dirs, &bytes_read));

        /* Root Entry gives sector_id chain of the Ministream storage */
        if((sector_id == first_sector_id) && (0 != p_compound_file->hdr._csectMiniFat))
            p_compound_file->ministream_chain_first_sector_id = file_dirs->_sectStart;

        for(i = 0; i < dirs_per_sector; i++)
        {
            PC_StructuredStorageDirectoryEntry file_dir = &file_dirs[i];

            if(CFBF_STGTY_INVALID != file_dir->_mse)
                n_directory_entries++;
        }
    }

    /* allocate core for these entries */
    if(NULL == (p_compound_file->decoded_directory_list = al_ptr_calloc_elem(COMPOUND_FILE_DECODED_DIRECTORY, n_directory_entries, &status)))
        return(status);

    /* now get all the entries */
    for(sector_id = first_sector_id;
        sector_id >= 0;
        sector_id = compound_file_get_next_sector_id(p_compound_file, sector_id))
    {
        U32 bytes_read;

        status_assert(compound_file_read_file_sector(p_compound_file, sector_id, file_dirs, &bytes_read));

        for(i = 0; i < dirs_per_sector; i++)
        {
            PC_StructuredStorageDirectoryEntry file_dir = &file_dirs[i];

            if(CFBF_STGTY_INVALID != file_dir->_mse)
                compound_file_decode_info_from_file_dir(p_compound_file, file_dir);
        }
    }

#if TRACE_ALLOWED
    compound_file_dump_decoded_directory_list(p_compound_file);
#endif

    return(status);
}

/* end of cfbf.c */
