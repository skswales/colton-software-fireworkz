/* cfbf.c */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) access functions */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/cfbf.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

_Check_return_
static STATUS
compound_file_read_file_header(
    _InoutRef_  P_COMPOUND_FILE p_compound_file);

_Check_return_
static STATUS
compound_file_read_DIFAT(
    _InoutRef_  P_COMPOUND_FILE p_compound_file);

_Check_return_
static STATUS
compound_file_create_directory_list(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT first_sector_id);

_Check_return_
static STATUS
compound_file_create_common(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    p_compound_file->cached_FAT_sector_id = CFBF_ENDOFCHAIN;

    p_compound_file->cached_mini_stream_FAT_sector_id = CFBF_ENDOFCHAIN;

    p_compound_file->mini_stream_chain_first_sector_id = CFBF_ENDOFCHAIN;

    status_return(compound_file_read_file_header(p_compound_file));

    status_return(compound_file_read_DIFAT(p_compound_file));

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

        if(NULL != p_compound_file->p_sector_buffer)
            al_ptr_free(p_compound_file->p_sector_buffer);

        if(NULL != p_compound_file->full_DIFAT)
            al_ptr_free(p_compound_file->full_DIFAT);

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

/* only reads 0x200 bytes from file even for 4096 byte sector - rest is zero padding */

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
            trace_1(TRACE_MODULE_CFBF, TEXT("  offset 0x%.2X|"), i * sizeof32(U32));
        trace_1(TRACE_MODULE_CFBF, TEXT("| %.8X|"), ((PC_U32) hdr)[i]);
    }
    } /*block*/
#endif

    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uMinorVersion=" U16_TFMT), hdr->_uMinorVersion);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uDllVersion=" U16_TFMT), hdr->_uDllVersion);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _uByteOrder=" U16_XTFMT), hdr->_uByteOrder);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _csectFat=" U32_TFMT), hdr->_csectFat);
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: _sectDirStart=" U32_TFMT), hdr->_sectDirStart);

    p_compound_file->sector_size = (U32) 1 << hdr->_uSectorShift;
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: sector_size=" U32_TFMT), p_compound_file->sector_size);

    p_compound_file->mini_stream_sector_size = (U32) 1 << hdr->_uMiniSectorShift;
    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageHeader: mini_stream_sector_size=" U32_TFMT), p_compound_file->mini_stream_sector_size);

    if(hdr->_uSectorShift > (9+3))
    {   /* failed plausibility check (_uSectorShift typically 9) */
        return(status_check());
    }

    if(hdr->_uMiniSectorShift > 6)
    {   /* failed plausibility check (_uMiniSectorShift typically 6) */
        assert(hdr->_uMiniSectorShift == 6);
        return(status_check());
    }

    assert(p_compound_file->sector_size <= COMPOUND_FILE_MAX_SECTOR_SIZE); /* for fixed block arrays at the moment */

    if(NULL == (p_compound_file->p_sector_buffer = al_ptr_alloc_bytes(P_BYTE, p_compound_file->sector_size, &status)))
        return(status);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
compound_file_read_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT sector_id,
    P_ANY dest,
    _OutRef_    P_U32 p_bytesread)
{
    const U32 actual_sector = (1 /*StructuredStorageHeader*/ + sector_id);
    const U32 actual_filepos_lo = actual_sector * p_compound_file->sector_size; /* might overflow! use uint32_multiply_check_overflow() */
    const U32 bytes_to_read = p_compound_file->sector_size;
    U32 bytes_read = 0;

    trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_sector(sector_id %u, filepos ") U32_XTFMT TEXT(")"), sector_id, actual_filepos_lo);
    assert(sector_id <= CFBF_MAXREGSECT);

    if(NULL != p_compound_file->file_handle)
    {
        STATUS status;

        status = file_seek(p_compound_file->file_handle, actual_filepos_lo, NULL /*hi*/, SEEK_SET);

        if(status_ok(status))
            status = file_read_bytes(dest, bytes_to_read, &bytes_read, p_compound_file->file_handle);

        if(status_fail(status))
            bytes_read = 0; /* in case file_seek() etc. fails */

        *p_bytesread = bytes_read;
        return(status);
    }

    if(0 != p_compound_file->data_size)
    {
        if(actual_filepos_lo <= p_compound_file->data_size) /* starting read within the data? */
        {
            U32 end_filepos = actual_filepos_lo + bytes_to_read; /* oh for access to CPU flags... */

            if( (end_filepos >= actual_filepos_lo) && (end_filepos >= bytes_to_read) ) /* check overflow */
            {
                if(end_filepos <= p_compound_file->data_size) /* ending read within the data? */
                    bytes_read = bytes_to_read; /* can satisfy whole request */
                else
                {
                    bytes_read = (p_compound_file->data_size - actual_filepos_lo); /* limit request to what we have */
                    trace_3(TRACE_MODULE_CFBF, TEXT("compound_file_read_sector(sector_id %u, filepos ") U32_XTFMT TEXT(") - *** limit to " U32_XTFMT), sector_id, actual_filepos_lo, bytes_read);
                }

                *p_bytesread = bytes_read;
                memcpy32(dest, p_compound_file->p_data + actual_filepos_lo, bytes_read);
                return(STATUS_OK);
            }
        }
        assert(actual_filepos_lo <= p_compound_file->data_size);
        assert(actual_filepos_lo + bytes_to_read <= p_compound_file->data_size);
    }

    *p_bytesread = 0;
    return(status_check());
}

_Check_return_
extern STATUS
compound_file_read_mini_stream_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT mini_stream_sector_id,
    P_ANY dest)
{
    U32 mini_stream_filepos = (U32) mini_stream_sector_id * p_compound_file->mini_stream_sector_size; /* mini stream limited in size so this won't overflow */
    U32 this_mini_stream_filepos = 0;
    CFBF_SECT mini_stream_chain_sector_id;

    trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_mini_stream_sector(mini_stream_sector_id %d): mini_stream_filepos ") U32_XTFMT, mini_stream_sector_id, mini_stream_filepos);
    assert(mini_stream_sector_id <= CFBF_MAXREGSECT);

    for(mini_stream_chain_sector_id = p_compound_file->mini_stream_chain_first_sector_id;
        mini_stream_chain_sector_id <= CFBF_MAXREGSECT;
        mini_stream_chain_sector_id = compound_file_get_next_sector_id(p_compound_file, mini_stream_chain_sector_id))
    {
        if(mini_stream_filepos < (this_mini_stream_filepos + p_compound_file->sector_size))
        {
            U32 bytes_read;

            trace_2(TRACE_MODULE_CFBF, TEXT("compound_file_read_mini_stream_sector(mini_stream_sector_id %d): this_mini_stream_filepos ") U32_XTFMT, mini_stream_sector_id, this_mini_stream_filepos);
            status_return(compound_file_read_sector(p_compound_file, mini_stream_chain_sector_id, p_compound_file->p_sector_buffer, &bytes_read));

            memcpy32(dest, p_compound_file->p_sector_buffer + (mini_stream_filepos - this_mini_stream_filepos), p_compound_file->mini_stream_sector_size);

            return(STATUS_DONE);
        }

        this_mini_stream_filepos += p_compound_file->sector_size;
    }

    return(STATUS_OK);
}

#if TRACE_ALLOWED

static void
compound_file_dump_DIFAT_file_header(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    const CFBF_SECT * x = p_compound_file->full_DIFAT;
    const U32 DIFAT_entries = elemof32(p_compound_file->hdr._sectFat); /* 109 */
    U32 list_index;
    BOOL needs_newline = TRUE;
    PTR_ASSERT(x);
    trace_1(TRACE_MODULE_CFBF, TEXT("DIFAT from file header (%u entries):|"), DIFAT_entries);
    for(list_index = 0; list_index < DIFAT_entries; list_index++, x++)
    {
        if(0 == (list_index % 8))
        {
            trace_1(TRACE_MODULE_CFBF, TEXT("  entry %.3u|"), list_index);
            needs_newline = FALSE;
        }
        else
            needs_newline = TRUE;
        trace_1(TRACE_MODULE_CFBF, TEXT("| %.8X|"), *x);
    }
    if(needs_newline)
        trace_0(TRACE_MODULE_CFBF, TEXT(""));

#if 1
    /* dump each of the referenced DIFAT sectors */
    for(list_index = 0; list_index < DIFAT_entries; list_index++)
    {
        const CFBF_SECT sector_id = p_compound_file->full_DIFAT[list_index];
        U32 bytes_read;
        U32 data_index;

        if(sector_id > CFBF_MAXREGSECT)
            break;

        trace_2(TRACE_MODULE_CFBF, TEXT("DIFAT entry %.3u, sector_id %d\n"), list_index, sector_id);
        status_assert(compound_file_read_sector(p_compound_file, sector_id, p_compound_file->p_sector_buffer, &bytes_read));
        assert(bytes_read == p_compound_file->sector_size);

        x = (const CFBF_SECT *) p_compound_file->p_sector_buffer;
        needs_newline = TRUE;
        for(data_index = 0; data_index < p_compound_file->sector_size/sizeof(CFBF_SECT); data_index++, x++)
        {
            if(0 == (data_index % 8))
            {
                trace_1(TRACE_MODULE_CFBF, TEXT("  entry %.3u|"), data_index);
                needs_newline = FALSE;
            }
            else
                needs_newline = TRUE;
            trace_1(TRACE_MODULE_CFBF, TEXT("| %.8X|"), *x);
        }
        if(needs_newline)
            trace_0(TRACE_MODULE_CFBF, TEXT(""));

#if 1 /* just dump the first FAT sector unless deep debugging! (which we will do for files with additional DIF blocks) */
        if(0 == p_compound_file->hdr._csectDif)
        {
            if(list_index + 1 < DIFAT_entries)
                trace_2(TRACE_MODULE_CFBF, TEXT("DIFAT entries %.3u..%.3u not dumped\n"), list_index + 1, DIFAT_entries - 1);

            break;
        }
#endif
    }
#endif
}

static void
compound_file_dump_file_DIFAT_DIF(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_FSINDEX i,
                PC_BYTE p)
{
    const CFBF_SECT * x = (const CFBF_SECT *) p;
    const U32 DIFAT_entry_bytes = p_compound_file->sector_size - sizeof32(CFBF_SECT); /* didn't copy the sector link word */
    const U32 DIFAT_entries = DIFAT_entry_bytes/sizeof32(*x);
    U32 list_index;
    BOOL needs_newline = TRUE;
    trace_2(TRACE_MODULE_CFBF, TEXT("DIFAT from file DIF (part %u, %u entries):"), i, DIFAT_entries);
    for(list_index = 0; list_index < DIFAT_entries; list_index++, x++)
    {
        if(0 == (list_index % 8))
        {
            trace_2(TRACE_MODULE_CFBF, TEXT("DIF %u entry %.3u|"), i, list_index);
            needs_newline = FALSE;
        }
        else
            needs_newline = TRUE;
        trace_1(TRACE_MODULE_CFBF, TEXT("| %.8X|"), *x);
    }
    if(needs_newline)
        trace_0(TRACE_MODULE_CFBF, TEXT(""));
}

#endif /*TRACE_ALLOWED*/

_Check_return_
static STATUS
compound_file_read_DIFAT(
    _InoutRef_  P_COMPOUND_FILE p_compound_file)
{
    STATUS status;
    U32 DIFAT_entry_bytes;

    if(NULL == (p_compound_file->full_DIFAT =
                al_ptr_calloc_bytes(CFBF_SECT *, sizeof32(p_compound_file->hdr._sectFat) + (p_compound_file->sector_size * (U32) p_compound_file->hdr._csectDif), &status)))
        return(status);

    /* load the first 109 DIFAT entries from the file header */
    DIFAT_entry_bytes = sizeof32(p_compound_file->hdr._sectFat);
    memcpy32(p_compound_file->full_DIFAT, p_compound_file->hdr._sectFat, DIFAT_entry_bytes);

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_MODULE_CFBF))
        compound_file_dump_DIFAT_file_header(p_compound_file);
#endif

    { /* append the rest of the DIFAT entries from the file (a FAT chain) */
    P_BYTE p = PtrAddBytes(P_BYTE, p_compound_file->full_DIFAT, DIFAT_entry_bytes);
    CFBF_FSINDEX i;
    CFBF_SECT sector_id = p_compound_file->hdr._sectDifStart;

    for(i = 0; i < p_compound_file->hdr._csectDif; i++)
    {
        U32 bytes_read;

        if( (CFBF_ENDOFCHAIN == sector_id) || (CFBF_FREESECT == sector_id) )
            break; /* NB some writers use CFBF_FREESECT - naughty! This is how LibreOffice handles them */

        assert(sector_id <= CFBF_MAXREGSECT);
        status_assert(compound_file_read_sector(p_compound_file, sector_id, p_compound_file->p_sector_buffer, &bytes_read));
        assert(bytes_read == p_compound_file->sector_size);

        DIFAT_entry_bytes = p_compound_file->sector_size - sizeof32(int); /* don't copy the sector link word */
        memcpy32(p, p_compound_file->p_sector_buffer, DIFAT_entry_bytes);

#if TRACE_ALLOWED
        if_constant(tracing(TRACE_MODULE_CFBF))
            compound_file_dump_file_DIFAT_DIF(p_compound_file, i, p);
#endif

        p += DIFAT_entry_bytes;

        sector_id = * (const CFBF_SECT *) (p_compound_file->p_sector_buffer + DIFAT_entry_bytes); /* chain using the sector link word */
    }

    assert((CFBF_ENDOFCHAIN == sector_id) || (CFBF_FREESECT == sector_id));
    } /*block*/

    return(status);
}

/*
get the next sector_id in this object's chain
*/

_Check_return_
extern CFBF_SECT /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT sector_id)
{
    const U32 FAT_index       = sector_id / (p_compound_file->sector_size/sizeof32(CFBF_SECT));
    const U32 FAT_block_index = sector_id % (p_compound_file->sector_size/sizeof32(CFBF_SECT));

    { /* ensure that the FAT sector containing this sector_id is loaded */
    const CFBF_SECT FAT_sector_id = p_compound_file->full_DIFAT[FAT_index];

    if(p_compound_file->cached_FAT_sector_id != FAT_sector_id)
    {   /* not currently cached, so must reset and load */
        U32 bytes_read;

        p_compound_file->cached_FAT_sector_id = CFBF_ENDOFCHAIN;

        if(status_ok(compound_file_read_sector(p_compound_file, FAT_sector_id, p_compound_file->cached_FAT_block, &bytes_read)))
        {
            assert(bytes_read == p_compound_file->sector_size);
            p_compound_file->cached_FAT_sector_id = FAT_sector_id;
        }

        if(CFBF_ENDOFCHAIN == p_compound_file->cached_FAT_sector_id)
            return(CFBF_ENDOFCHAIN);
    }
    } /*block*/

    {
    const CFBF_SECT next_sector_id = p_compound_file->cached_FAT_block[FAT_block_index];
    trace_2(TRACE_MODULE_CFBF, TEXT("sector_id %d -> next sector_id %d"), sector_id, next_sector_id);
    return(next_sector_id);
    } /*block*/
}

_Check_return_
extern CFBF_SECT /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_mini_stream_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT mini_stream_sector_id)
{
    /* NB mini stream FAT is stored in standard sectors */
    const U32 mini_stream_FAT_index       = mini_stream_sector_id / (p_compound_file->sector_size/sizeof32(CFBF_SECT));
    const U32 mini_stream_FAT_block_index = mini_stream_sector_id % (p_compound_file->sector_size/sizeof32(CFBF_SECT));

    consume(U32, mini_stream_FAT_index);
    assert(mini_stream_FAT_index == 0);
    assert(p_compound_file->hdr._csectMiniFat == 1);

    { /* ensure that the mini stream FAT sector containing this sector_id is loaded */
    const CFBF_SECT mini_stream_FAT_sector_id = p_compound_file->hdr._sectMiniFatStart;

    if(p_compound_file->cached_mini_stream_FAT_sector_id != mini_stream_FAT_sector_id)
    {   /* not currently cached, so must reset and load */
        U32 bytes_read;

        p_compound_file->cached_mini_stream_FAT_sector_id = CFBF_ENDOFCHAIN;

        if(status_ok(compound_file_read_sector(p_compound_file, mini_stream_FAT_sector_id, p_compound_file->cached_mini_stream_FAT_block, &bytes_read)))
        {
            assert(bytes_read == p_compound_file->sector_size);
            p_compound_file->cached_mini_stream_FAT_sector_id = mini_stream_FAT_sector_id;
        }

        if(CFBF_ENDOFCHAIN == p_compound_file->cached_mini_stream_FAT_sector_id)
            return(CFBF_ENDOFCHAIN);
    }
    } /*block*/

    {
    const CFBF_SECT next_mini_stream_sector_id = p_compound_file->cached_mini_stream_FAT_block[mini_stream_FAT_block_index];
    trace_2(TRACE_MODULE_CFBF, TEXT("mini_stream_sector_id %d -> next mini_stream_sector_id %d"), mini_stream_sector_id, next_mini_stream_sector_id);
    return(next_mini_stream_sector_id);
    } /*block*/
}

#if TRACE_ALLOWED

static void
compound_file_dump_directory_entry(
    _InRef_     P_COMPOUND_FILE p_compound_file,
    _InRef_     PC_StructuredStorageDirectoryEntry file_dir,
    _InVal_     U32 dirnum)
{
    UNREFERENCED_PARAMETER_InRef_(p_compound_file);

    if(CFBF_STGTY_INVALID == file_dir->_mse)
        return;

    trace_1(TRACE_MODULE_CFBF, TEXT("StructuredStorageDirectoryEntry #%u |"), dirnum);

    { /* dump this SSDE */
    U32 i;
    for(i = 0; i < sizeof32(StructuredStorageDirectoryEntry)/sizeof32(int); i++)
    {
        if(0 == (i & (4-1)))
            trace_1(TRACE_MODULE_CFBF, TEXT("  offset 0x%.2X|"), i * sizeof32(int));
        trace_1(TRACE_MODULE_CFBF, TEXT("| %.8X|"), ((const int *) file_dir)[i]);
    }
    } /*block*/

    trace_2(TRACE_MODULE_CFBF, TEXT("  _mse=%u(%s) |"),
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

    if(readval_U16_LE(p) < CH_SPACE)
    {
        U16 unknown = readval_U16_LE(p);
        trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), unknown);
        p += 2; /* step over unknown short */
        i += 2;
    }
    for( ; i < file_dir->_cb/2; i++, p += 2)
    {
        if(CH_NULL == p[1])
            trace_1(TRACE_MODULE_CFBF, TEXT("|%c|"), p[0]);
        else
            trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), readval_U16_LE(p));
    }

#elif WINDOWS

    const WCHAR * p = (const WCHAR *) file_dir->_ab;

    if(readval_U16_LE(p) < CH_SPACE)
    {
        U16 unknown = readval_U16_LE(p);
        trace_1(TRACE_MODULE_CFBF, TEXT("|[%.4X]|"), unknown);
        p += 1; /* step over unknown short */
        i += 2;
    }

    __pragma(warning(suppress: 28182))
    for( ; i < file_dir->_cb/2; i++, p++)
    {
        trace_1(TRACE_MODULE_CFBF, TEXT("|%lc|"), readval_U16_LE(p));
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
        *q++ = readval_U16_LE(p);
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
        {
            consume_int(xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("STREAM  %8u @%4u %s"),
                                  decoded_directory->_ulSize,
                                  decoded_directory->_sectStart,
                                  (decoded_directory->_ulSize < p_compound_file->hdr._ulMiniSectorCutoff) ? "Mini stream" : "FAT"));
        }
        else if(CFBF_STGTY_STORAGE == decoded_directory->_mse)
            consume_int(xsnprintf(&buffer[offset], elemof32(buffer) - offset, ("STORAGE ")));

        trace_0(TRACE_MODULE_CFBF, report_sbstr(buffer));
    }
}

#endif /* TRACE_ALLOWED */

_Check_return_
static STATUS
compound_file_create_directory_list(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT first_sector_id)
{
    STATUS status;
    P_StructuredStorageDirectoryEntry file_dirs = (P_StructuredStorageDirectoryEntry) p_compound_file->p_sector_buffer; /* always read via this sector-size buffer*/
    const U32 dirs_per_sector = p_compound_file->sector_size / sizeof32(StructuredStorageDirectoryEntry);
    U32 n_directory_entries = 0;
    CFBF_SECT sector_id;
    U32 i;

    /* first get the size of the entries */
    for(sector_id = first_sector_id;
        sector_id <= CFBF_MAXREGSECT;
        sector_id = compound_file_get_next_sector_id(p_compound_file, sector_id))
    {
        U32 bytes_read;

        status_assert(compound_file_read_sector(p_compound_file, sector_id, file_dirs, &bytes_read));

        /* Root Entry gives sector_id chain of the mini stream storage */
        if( (sector_id == first_sector_id) && (0 != p_compound_file->hdr._csectMiniFat) )
            p_compound_file->mini_stream_chain_first_sector_id = file_dirs->_sectStart;

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
        sector_id <= CFBF_MAXREGSECT;
        sector_id = compound_file_get_next_sector_id(p_compound_file, sector_id))
    {
        U32 bytes_read;

        status_assert(compound_file_read_sector(p_compound_file, sector_id, file_dirs, &bytes_read));

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
