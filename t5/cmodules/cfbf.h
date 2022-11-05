/* cfbf.h */

/* Compound File Binary Format (COM / OLE 2 Structured Storage) structures & access functions */

/*
see [MS-CFB]
https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-cfb/53989ce4-7b05-4f8d-829b-d08d6148375b
and the earlier
http://download.microsoft.com/download/0/B/E/0BE8BDD7-E5E8-422A-ABFD-4342ED7AD886/WindowsCompoundBinaryFileFormatSpecification.pdf
and
https://www.openoffice.org/sc/compdocfileformat.pdf
*/

typedef U32 CFBF_SECT;

#define /*CFBF_SECT*/ CFBF_MAXREGSECT   0xFFFFFFFAU /* regular sector numbers in [0..MAXREGSECT] */

/* Streams of a compound file are composed of
 * (zero-based sector_id) sector data blocks.
 * Some sector ids have special meaning:
 */
#define /*CFBF_SECT*/ CFBF_DIFSECT      0xFFFFFFFCU /* used by master sector allocation table */
#define /*CFBF_SECT*/ CFBF_FATSECT      0xFFFFFFFDU /* used by sector allocation table */
#define /*CFBF_SECT*/ CFBF_ENDOFCHAIN   0xFFFFFFFEU /* terminates the sector chain */
#define /*CFBF_SECT*/ CFBF_FREESECT     0xFFFFFFFFU /* not part of any stream */

typedef U32 CFBF_FSINDEX;

typedef U32 CFBF_STREAM_ID;

#define /*CFBF_STREAM_ID*/ CFBF_STREAM_ID_MAXREGSID 0xFFFFFFFAU /* regular stream IDs in [0..MAXREGSID] */
#define /*CFBF_STREAM_ID*/ CFBF_STREAM_ID_NOSTREAM  0xFFFFFFFFU /* terminator or empty pointer */

typedef struct CFBF_TIME_T
{
    U32 dwLowDateTime;
    U32 dwHighDateTime;
}
CFBF_TIME_T; /* a Windows FILETIME */

#define CFBF_FILE_HEADER_ID_BYTES 8

/*
Each compound file always begins with this 512-byte header
See WindowsCompoundBinaryFileFormatSpecification.pdf
If >512 byte sector size is specified, the StructuredStorageHeader is followed by zero padding (_sectFat[] isn't extended)
*/

typedef struct StructuredStorageHeader      /* [offset from start in bytes, length in bytes] */
{
    BYTE _abSig[CFBF_FILE_HEADER_ID_BYTES]; /*[000H,08]*/ /*0d..7d*/ /* {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1} for current version */
    BYTE _clid[16]; /* CLSID */             /*[008H,16]*/ /*8d..23d*/ /* class id (set with WriteClassStg, retrieved with GetClassFile/ReadClassStg) */

    U16 _uMinorVersion;                     /*[018H,02]*/ /*24d*/ /* minor version of the format: 33 is written by reference implementation */
    U16 _uDllVersion;                       /*[01AH,02]*/ /*26d*/ /* major version of the dll/format: 3 is written by reference implementation */

    U16 _uByteOrder;                        /*[01CH,02]*/ /*28d*/ /* 0xFFFE: indicates Intel byte-ordering */

    U16 _uSectorShift;                      /*[01EH,02]*/ /*30d*/ /* size of sectors in power-of-two (typically 9, indicating 512-byte sectors) */
    U16 _uMiniSectorShift;                  /*[020H,02]*/ /*32d*/ /* size of mini-sectors in power-of-two (typically 6, indicating 64-byte mini-sectors) */

    U16 _usReserved;                        /*[022H,02]*/ /*34d*/ /* reserved, must be zero */
    U32 _ulReserved1;                       /*[024H,04]*/ /*36d*/ /* reserved, must be zero */
    U32 _ulReserved2;                       /*[028H,04]*/ /*40d*/ /* reserved, must be zero */

    CFBF_FSINDEX _csectFat;                 /*[02CH,04]*/ /*44d*/ /* number of SECTs in the FAT chain */
    CFBF_SECT _sectDirStart;                /*[030H,04]*/ /*48d*/ /* first SECT in the Directory chain */

    U32 _signature; /* DFSIGNATURE */       /*[034H,04]*/ /*52d*/ /* signature used for transactioning: must be zero */

    U32 _ulMiniSectorCutoff;                /*[038H,04]*/ /*56d*/ /* maximum size for mini-streams: typically 4096 bytes */
    CFBF_SECT _sectMiniFatStart;            /*[03CH,04]*/ /*60d*/ /* first SECT in the mini-FAT chain */
    CFBF_FSINDEX _csectMiniFat;             /*[040H,04]*/ /*64d*/ /* number of SECTs in the mini-FAT chain */

    CFBF_SECT _sectDifStart;                /*[044H,04]*/ /*68d*/ /* first SECT in the DIF chain */
    CFBF_FSINDEX _csectDif;                 /*[048H,04]*/ /*72d*/ /* number of SECTs in the DIF chain */

    CFBF_SECT _sectFat[109];                /*[04CH,436]*/ /*76d..511d*/ /* the SECTs of the first 109 FAT sectors */
}                                           /*[200H]*/ /*512d*/
StructuredStorageHeader, * P_StructuredStorageHeader;

/* NB sector_id zero follows immediately after sector containing compound file header (file offset 512 or 4096) */

enum CFBF_STGTY
{
    CFBF_STGTY_INVALID      = 0,
    CFBF_STGTY_STORAGE      = 1,
    CFBF_STGTY_STREAM       = 2,
    CFBF_STGTY_LOCKBYTES    = 3,
    CFBF_STGTY_PROPERTY     = 4,
    CFBF_STGTY_ROOT         = 5
};

typedef struct StructuredStorageDirectoryEntry  /* [offset from start in bytes, length in bytes] */
{
    BYTE _ab[64];                               /*[000H,64]*/         /* The Element name in Unicode, padded with zeros */
    U16 _cb;                                    /*[040H,02]*/ /*64d*/ /* Length of the Element name in characters, not bytes */ /* Hmm. Methinks it is bytes - just look at any DocFile ... */

    BYTE _mse;                                  /*[042H,01]*/ /*66d*/ /* Type of object: value taken from the STGTY enumeration */

    BYTE _bflags;                               /*[043H,01]*/ /*67d*/ /* Value taken from DECOLOR enumeration */
#define CFBF_DECOLOR_RED        0 /* enum DECOLOR */
#define CFBF_DECOLOR_BLACK      1

    CFBF_STREAM_ID _sidLeftSib;                 /*[044H,04]*/ /*68d*/ /* SID of the left-sibling of this entry in the directory tree */
    CFBF_STREAM_ID _sidRightSib;                /*[048H,04]*/ /*72d*/ /* SID of the right-sibling of this entry in the directory tree */
    CFBF_STREAM_ID _sidChild;                   /*[04CH,04]*/ /*76d*/ /* SID of the child acting as the root of all the children of this element (if _mse=STGTY_STORAGE) */

    BYTE _clsId[16]; /*GUID*/                   /*[050H,16]*/ /*80d..95d*/ /* CLSID of this storage (if _mse=STGTY_STORAGE) */

    U32 _dwUserFlags;                           /*[060H,04]*/ /*96d*/ /* User flags of this storage (if _mse=STGTY_STORAGE) */

    U32 _time_0_dwLowDateTime; /* CFBF_TIME_T *//*[064H,16]*/ /*100d*/ /* Create/Modify time-stamps (if _mse=STGTY_STORAGE) (NB consider structure member packing) */
    U32 _time_0_dwHighDateTime;                 /*104d*/
    U32 _time_1_dwLowDateTime;                  /*108d*/
    U32 _time_1_dwHighDateTime;                 /*112d*/

    CFBF_SECT _sectStart;                       /*[074H,04]*/ /*116d*/ /* starting SECT of the stream (if _mse=STGTY_STREAM) */

    U32 _ulSize;                                /*[078H,04]*/ /*120d*/ /* size of stream in bytes (if _mse=STGTY_STREAM) */

    U32 _dptPropType; /* DFPROPTYPE */          /*[07CH,02]*/ /*124d*/ /* Reserved for future use. Must be zero */
                                                /*[07EH,02]*/ /*126d*/ /* NB I presume MS documentation missed this one */
}                                               /*[080H]*/ /*128d*/
StructuredStorageDirectoryEntry, * P_StructuredStorageDirectoryEntry; typedef const StructuredStorageDirectoryEntry * PC_StructuredStorageDirectoryEntry;

/*
cfbf.c
*/

typedef struct COMPOUND_FILE_DECODED_DIRECTORY
{
    union COMPOUND_FILE_DECODED_DIRECTORY_NAME
    {
        WCHARZ wchar[32];
#if !(WINDOWS || RISCOS)
        U8Z u8[64];
#endif /* OS */
    } name;

    /* these are directly copied from the StructuredStorageDirectoryEntry */
    BYTE _mse;                                  /* Type of object: value taken from the STGTY enumeration */
    CFBF_SECT _sectStart;                       /* starting SECT of the stream (if _mse=STGTY_STREAM) */
    U32 _ulSize;                                /* size of stream in bytes (if _mse=STGTY_STREAM) */
}
COMPOUND_FILE_DECODED_DIRECTORY;

#define COMPOUND_FILE_STANDARD_SECTOR_SIZE  512
#define COMPOUND_FILE_LARGE_SECTOR_SIZE     4096
#define COMPOUND_FILE_MAX_SECTOR_SIZE       4096

typedef struct COMPOUND_FILE
{
/*public members*/
    StructuredStorageHeader hdr;

    COMPOUND_FILE_DECODED_DIRECTORY * decoded_directory_list;
    U32 decoded_directory_count;

/*private to cfbf.c*/
    PC_BYTE p_data;
    U32 data_size;
    FILE_HANDLE file_handle;

    U32 sector_size; /* derived from file header _uSectorShift */
    P_BYTE p_sector_buffer; /* -> BYTE[sector_size] for reading */

    U32 mini_stream_sector_size; /* derived from file header _uMiniSectorShift */
    CFBF_SECT mini_stream_chain_first_sector_id; /* derived from Root Entry */

    CFBF_SECT * full_DIFAT; /* the full set of DIFAT entries (with sector links removed) */

    CFBF_SECT cached_FAT_block[COMPOUND_FILE_MAX_SECTOR_SIZE/sizeof32(CFBF_SECT)]; /* pertains to cached_FAT_sector_id */
    CFBF_SECT cached_FAT_sector_id;

    CFBF_SECT cached_mini_stream_FAT_block[COMPOUND_FILE_MAX_SECTOR_SIZE/sizeof32(CFBF_SECT)]; /* pertains to cached_mini_stream_FAT_sector_id */
    CFBF_SECT cached_mini_stream_FAT_sector_id;
}
COMPOUND_FILE, * P_COMPOUND_FILE, ** P_P_COMPOUND_FILE; /* no const as almost all calls modify */

_Check_return_
_Ret_maybenull_
extern P_COMPOUND_FILE
compound_file_create_from_data(
    _In_reads_(data_size) PC_BYTE p_data,
    _InVal_     U32 data_size,
    _OutRef_    P_STATUS p_status);

_Check_return_
_Ret_maybenull_
extern P_COMPOUND_FILE
compound_file_create_from_file_handle(
    _InRef_     FILE_HANDLE file_handle,
    _OutRef_    P_STATUS p_status);

extern void
compound_file_dispose(
    _InoutRef_  P_P_COMPOUND_FILE p_p_compound_file);

_Check_return_
extern BOOL
compound_file_file_header_id_test(
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes);

_Check_return_
extern STATUS
compound_file_read_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT sector_id,
    P_ANY dest,
    _OutRef_    P_U32 p_bytesread);

_Check_return_
extern STATUS
compound_file_read_mini_stream_sector(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT mini_stream_sector_id,
    P_ANY dest);

_Check_return_
extern CFBF_SECT /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT sector_id);

_Check_return_
extern CFBF_SECT /* zero-based sector_id or CFBF_ENDOFCHAIN */
compound_file_get_next_mini_stream_sector_id(
    _InoutRef_  P_COMPOUND_FILE p_compound_file,
    _InVal_     CFBF_SECT mini_stream_sector_id);

/*
cfbfwrite.c
*/

_Check_return_
extern STATUS
cfbf_write_stream_in_storage(
    _InRef_opt_ P_ARRAY_HANDLE p_array_handle_storage,
    _In_opt_z_  PCTSTR storage_filename,
    _InVal_     T5_FILETYPE storage_filetype,
    _In_z_      PCTSTR stream_name,
    _In_reads_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes);

/*
cfbfnative.c
*/

#if WINDOWS

_Check_return_
extern STATUS
stg_cfbf_read_stream_from_storage(
    _In_z_      PCWSTR wstr_storage_filename,
    _In_z_      PCWSTR wstr_stream_name,
    _OutRef_    P_ARRAY_HANDLE p_h_data);

_Check_return_
extern STATUS
stg_cfbf_write_stream_in_storage(
    _In_z_      PCWSTR wstr_storage_filename,
    _In_z_      PCWSTR wstr_stream_name,
    _In_reads_bytes_(n_bytes) PC_ANY p_data,
    _InVal_     U32 n_bytes);

#endif /* WINDOWS */

/* end of cfbf.h */
