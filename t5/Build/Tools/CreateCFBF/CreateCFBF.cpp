// CreateCFBF.cpp

#include <windows.h>

#include <stdio.h>

#define SS_NORMAL   512     // As created by StgCreateDocfile()
#define SS_LARGE    4096    // Needs v4 DLL (Windows 2000)

static void
byteswap(
    unsigned char * a,
    unsigned char * b,
    size_t n)
{
    while(n != 0)
    {
        unsigned char tmp = *b;
        *b++ = *a;
        *a++ = tmp;
        --n;
    }
}

/*
StgCreateDocfile() creates a file containing:
File Sector Zero: Structured Storage Header
File Sector One (Data Sector[0]): Sector Allocation Table
File Sector Two (Data Sector[1]): Directory

It's more convenient to store the SAT sectors contiguously
for extension, so reorder a copy to contain:
File Sector Zero: Structured Storage Header
File Sector One (Data Sector[0]): Directory
File Sector Two (Data Sector[1]): Sector Allocation Table
*/

static int
create_storage_512(void)
{
    int ok = EXIT_FAILURE;
    static const WCHAR filename[MAX_PATH] = L"cfbf-512,ffd";
    DWORD grfMode = STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE;
    IStorage * pstgOpen = NULL;
    HRESULT hr = StgCreateDocfile(filename, grfMode, 0 /*reserved*/, &pstgOpen);
    if(SUCCEEDED(hr))
    {
        pstgOpen->Release();
        ok = EXIT_SUCCESS;
    }
    return(ok);
}

static int
reorder_storage_512(void)
{
    int ok = EXIT_FAILURE;
    const char * infile = "cfbf-512,ffd";
    const char * outfile = "cfbf-512ro,ffd";
    const unsigned int sector_size = SS_NORMAL;
    unsigned char buffer[sector_size*3];
    FILE * fin;
    FILE * fout;

    if(0 == fopen_s(&fin, infile, "rb"))
    {
        if(0 == fopen_s(&fout, outfile, "wb"))
        {
            if (3 != fread_s(buffer, sizeof(buffer), sector_size, 3, fin))
            {
                fprintf_s(stderr, "Failed to read three sectors from %s\n", infile);
                goto barf;
            }

            /* Sector Zero: Structured Storage Header */
            /* Swap sector ids of SAT and Directory for reorder */
            byteswap(&buffer[0*sector_size + 0x4C /*_sectFat[0]*/   /*[04CH,436]*/ /*76d..511d*/ /* the SECTs of the first 109 FAT sectors */],
                     &buffer[0*sector_size + 0x30 /*_sectDirStart*/ /*[030H,04]*/ /*48d*/ /* first SECT in the Directory chain */],
                     4);

            /* Sector One: SAT */
            /* Swap sector ids of SAT and Directory for reorder */
            byteswap(&buffer[1*sector_size + 0],
                     &buffer[1*sector_size + 4],
                     4);

            /* Sector Two: Directory */

            /* Write out, reordered as Header, Directory, SAT */
            if(1 != fwrite(&buffer[0*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector zero to %s\n", outfile);
                goto barf;
            }
            if(1 != fwrite(&buffer[2*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector one to %s\n", outfile);
                goto barf;
            }
            if(1 != fwrite(&buffer[1*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector two to %s\n", outfile);
                goto barf;
            }

            ok = EXIT_SUCCESS;

        barf:;

            (void) fclose(fout);
        }
        (void) fclose(fin);
    }

    return(ok);
}

static int
create_storage_4096(void)
{
    int ok = EXIT_FAILURE;
    static const WCHAR filename[MAX_PATH] = L"cfbf-4096,ffd";
    DWORD grfMode = STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE;
    STGFMT stgfmt = STGFMT_DOCFILE;
    DWORD grfAttrs = 0;
    STGOPTIONS stgoptions = { 1 /*usVersion*/, 0 /*usPadding*/, SS_LARGE /*ulSectorSize*/ };
    IStorage * pstgOpen = NULL;
    HRESULT hr = StgCreateStorageEx(filename, grfMode, stgfmt, grfAttrs, &stgoptions, NULL /*pSecurityDescriptor*/, IID_IStorage, reinterpret_cast<void**>(&pstgOpen));
    if(SUCCEEDED(hr))
    {
        pstgOpen->Release();
        ok = EXIT_SUCCESS;
    }
    return(ok);
}

static int
reorder_storage_4096(void)
{
    int ok = EXIT_FAILURE;
    const char * infile = "cfbf-4096,ffd";
    const char * outfile = "cfbf-4096ro,ffd";
    const unsigned int sector_size = SS_LARGE;
    unsigned char buffer[sector_size*3];
    FILE * fin;
    FILE * fout;

    if(0 == fopen_s(&fin, infile, "rb"))
    {
        if(0 == fopen_s(&fout, outfile, "wb"))
        {
            if (3 != fread_s(buffer, sizeof(buffer), sector_size, 3, fin))
            {
                fprintf_s(stderr, "Failed to read three sectors from %s\n", infile);
                goto barf;
            }

            /* Sector Zero: Structured Storage Header */
            /* Swap sector ids of SAT and Directory for reorder */
            byteswap(&buffer[0*sector_size + 0x4C /*_sectFat[0]*/   /*[04CH,436]*/ /*76d..511d*/ /* the SECTs of the first 109 FAT sectors */],
                     &buffer[0*sector_size + 0x30 /*_sectDirStart*/ /*[030H,04]*/ /*48d*/ /* first SECT in the Directory chain */],
                     4);

            /* Sector One: SAT */
            /* Swap sector ids of SAT and Directory for reorder */
            byteswap(&buffer[1*sector_size + 0],
                     &buffer[1*sector_size + 4],
                     4);

            /* Sector Two: Directory */

            /* Write out, reordered as Header, Directory, SAT */
            if(1 != fwrite(&buffer[0*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector zero to %s\n", outfile);
                goto barf;
            }
            if(1 != fwrite(&buffer[2*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector one to %s\n", outfile);
                goto barf;
            }
            if(1 != fwrite(&buffer[1*sector_size], sector_size, 1, fout))
            {
                fprintf_s(stderr, "Failed to write sector two to %s\n", outfile);
                goto barf;
            }

            ok = EXIT_SUCCESS;

        barf:;

            (void) fclose(fout);
        }
        (void) fclose(fin);
    }

    return(ok);
}

int main()
{
    int ok;

    if(EXIT_SUCCESS != (ok = create_storage_512())) return(ok);
    if(EXIT_SUCCESS != (ok = reorder_storage_512())) return(ok);

    if(EXIT_SUCCESS != (ok = create_storage_4096())) return(ok);
    if(EXIT_SUCCESS != (ok = reorder_storage_4096())) return(ok);

    return(EXIT_SUCCESS);
}

// end of CreateCFBF.cpp