/* fs_xls_savex.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2020 Stuart Swales */

/* Excel spreadsheet XML format saver */

/* SKS April 2014 */

#include "common/gflags.h"

#include "fs_xls/fs_xls.h"

#include "ob_skel/ff_io.h"

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

/******************************************************************************
*
* Save as Microsoft Office Excel 2003 XML Format
*
******************************************************************************/

static COL g_s_col, g_e_col;
static ROW g_s_row, g_e_row;

#define QUOTE_STR "\""

static U32 xml_level = 2;

_Check_return_
static STATUS
xml_output_tag(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR p_string)
{
    U32 i;

    for(i = 0; i < xml_level; ++i)
        status_return(plain_write_ustr(p_ff_op_format, USTR_TEXT("  ")));

    return(plain_write_ustr(p_ff_op_format, p_string));
}

_Check_return_
static STATUS
xls_xml_output_table(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U32 cols,
    _InVal_     U32 rows)
{
    U8 buffer[32];

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Table")));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(" " "ss:ExpandedColumnCount=")));
    consume_int(xsnprintf(buffer, elemof32(buffer), QUOTE_STR "%u" QUOTE_STR, cols));
    status_return(plain_write_ustr(p_ff_op_format, ustr_bptr(buffer)));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(" " "ss:ExpandedRowCount=")));
    consume_int(xsnprintf(buffer, elemof32(buffer), QUOTE_STR "%u" QUOTE_STR, rows));
    status_return(plain_write_ustr(p_ff_op_format, ustr_bptr(buffer)));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(" " "x:FullColumns=" QUOTE_STR "1" QUOTE_STR
                  " " "x:FullRows="    QUOTE_STR "1" QUOTE_STR)));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(">" "\n")));

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_xml_output_row(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U32 row)
{
    UCHARZ buffer[32];

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Row"
                      " " "ss:Index=")));
    consume_int(xsnprintf(buffer, elemof32(buffer), QUOTE_STR "%u" QUOTE_STR, 1U + row));
    status_return(plain_write_ustr(p_ff_op_format, ustr_bptr(buffer)));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(">" "\n")));

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_xml_output_cell(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U32 col,
    _In_reads_opt_(formula_len) PC_USTR ustr_formula,
    _InVal_     U32 formula_len)
{
    UCHARZ buffer[32];

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Cell"
                      " " "ss:Index=")));
    consume_int(xsnprintf(buffer, elemof32(buffer), QUOTE_STR "%u" QUOTE_STR, 1U + col));
    status_return(plain_write_ustr(p_ff_op_format, ustr_bptr(buffer)));

    if((0 != formula_len) && (NULL != ustr_formula))
    {
        status_return(plain_write_ustr(p_ff_op_format,
            USTR_TEXT(" " "ss:Formula=" QUOTE_STR)));

        status_return(plain_write_uchars(p_ff_op_format, ustr_formula, formula_len));

        status_return(plain_write_ustr(p_ff_op_format,
            USTR_TEXT(QUOTE_STR)));
    }

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT(">" "\n")));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a number to a Microsoft Office Excel 2003 XML Format file
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_number(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_contents)
{
    const U32 contents_len = quick_ublock_bytes(p_quick_ublock_contents);

    status_return(xls_xml_output_cell(p_ff_op_format, p_object_data->data_ref.arg.slr.col, NULL, 0));

    xml_level++;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Data"
                      " " "ss:Type=" QUOTE_STR "Number" QUOTE_STR
                      ">" /* no newline */)));

    status_return(plain_write_uchars(p_ff_op_format, quick_ublock_uchars(p_quick_ublock_contents), contents_len));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT("</Data>" "\n")));

    xml_level--;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("</Cell>" "\n")));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a cell as a label to a Microsoft Office Excel 2003 XML Format file
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_label(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    const U32 contents_len = quick_ublock_bytes(p_quick_ublock);

    status_return(xls_xml_output_cell(p_ff_op_format, p_object_data->data_ref.arg.slr.col, NULL, 0));

    xml_level++;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Data"
                      " " "ss:Type=" QUOTE_STR "String" QUOTE_STR
                      ">" /* no newline */)));

    status_return(plain_write_uchars(p_ff_op_format, quick_ublock_uchars(p_quick_ublock), contents_len));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT("</Data>" "\n")));

    xml_level--;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("</Cell>" "\n")));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a formula to a Microsoft Office Excel 2003 XML Format file
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_formula(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_formula,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_contents)
{
    const U32 formula_len = quick_ublock_bytes(p_quick_ublock_formula);
    const U32 contents_len = quick_ublock_bytes(p_quick_ublock_contents);

    status_return(xls_xml_output_cell(p_ff_op_format, p_object_data->data_ref.arg.slr.col, quick_ublock_uchars(p_quick_ublock_formula), formula_len));

    xml_level++;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("<Data"
                      " " "ss:Type=" QUOTE_STR "Number" QUOTE_STR
                      ">")));

    status_return(plain_write_uchars(p_ff_op_format, quick_ublock_uchars(p_quick_ublock_contents), contents_len));

    status_return(plain_write_ustr(p_ff_op_format,
        USTR_TEXT("</Data>" "\n")));

    xml_level--;

    status_return(xml_output_tag(p_ff_op_format,
        USTR_TEXT("</Cell>" "\n")));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a TEXT cell to a Microsoft Office Excel 2003 XML Format file
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_cell_text(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_OBJECT_DATA p_object_data)
{
    STATUS status = STATUS_OK;
    OBJECT_DATA_READ object_data_read;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    object_data_read.object_data = *p_object_data;
    status = object_call_id(object_data_read.object_data.object_id,
                            p_docu, T5_MSG_OBJECT_DATA_READ, &object_data_read);

    status_return(status);

    {
    OBJECT_READ_TEXT object_read_text;
    object_read_text.p_quick_ublock = &quick_ublock;
    object_read_text.object_data = *p_object_data;
    object_read_text.type = OBJECT_READ_TEXT_PLAIN;
    status = object_call_id(object_read_text.object_data.object_id,
                            p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);
    } /*block*/

    if(status_ok(status))
        status = xls_xml_write_label(p_ff_op_format, p_object_data, &quick_ublock);

    quick_ublock_dispose(&quick_ublock);

    ss_data_free_resources(&object_data_read.ss_data);

    return(status);
}

/******************************************************************************
*
* write out a SS cell to a Microsoft Office Excel 2003 XML Format file
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_cell_ss(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_OBJECT_DATA p_object_data)
{
    STATUS status = STATUS_OK;
    P_EV_CELL p_ev_cell = p_object_data->u.p_ev_cell;

    if(P_DATA_NONE != p_ev_cell)
    {
        const EV_DOCNO ev_docno = (EV_DOCNO) docno_from_p_docu(p_docu);
        QUICK_UBLOCK_WITH_BUFFER(contents_data_quick_ublock, 64);
        QUICK_UBLOCK_WITH_BUFFER(formula_data_quick_ublock, 64);
        quick_ublock_with_buffer_setup(contents_data_quick_ublock);
        quick_ublock_with_buffer_setup(formula_data_quick_ublock);

        if(p_ev_cell->ev_parms.data_only)
        {
        }
        else
        {
            status = ev_decompile(&formula_data_quick_ublock, p_ev_cell, 0, ev_docno);
        }

        if(status_ok(status))
        {
            SS_DATA ss_data;
            ss_data_from_ev_cell(&ss_data, p_ev_cell);
            status = ss_data_decode(&contents_data_quick_ublock, &ss_data, ev_docno);
        }

        if(0 != quick_ublock_bytes(&formula_data_quick_ublock))
        {
            status = xls_xml_write_formula(p_ff_op_format, p_object_data, &formula_data_quick_ublock, &contents_data_quick_ublock);
        }
        else
        {
            status = xls_xml_write_number(p_ff_op_format, p_object_data, &contents_data_quick_ublock);
        }
    }

    return(status);
}

_Check_return_
static STATUS
xls_xml_write_bof(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    PC_USTR str = (PC_USTR)
"<?xml version=\"1.0\"?>" "\n"
"<?mso-application progid=\"Excel.Sheet\"?>" "\n"

"<Workbook" "\n"
"   xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"" "\n"
"   xmlns:o=\"urn:schemas-microsoft-com:office:office\"" "\n"
"   xmlns:x=\"urn:schemas-microsoft-com:office:excel\"" "\n"
"   xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"" "\n"
"   xmlns:html=\"http://www.w3.org/TR/REC-html40\">" "\n"

"  <DocumentProperties xmlns=\"urn:schemas-microsoft-com:office:office\">" "\n"
"    <Author>Stuart Swales</Author>" "\n"
"    <LastAuthor>Bill Gates</LastAuthor>" "\n"
"    <Created>2007-03-15T23:04:04Z</Created>" "\n"
"    <Company>Colton Software</Company>" "\n"
"    <Version>11.8036</Version>" "\n"
"  </DocumentProperties>" "\n"

"  <ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\">" "\n"
"    <WindowHeight>6795</WindowHeight>" "\n"
"    <WindowWidth>8460</WindowWidth>" "\n"
"    <WindowTopX>120</WindowTopX>" "\n"
"    <WindowTopY>15</WindowTopY>" "\n"
"    <ProtectStructure>False</ProtectStructure>" "\n"
"    <ProtectWindows>False</ProtectWindows>" "\n"
"  </ExcelWorkbook>" "\n"

"  <Styles>" "\n"
"    <Style ss:ID=\"Default\" ss:Name=\"Normal\">" "\n"
"      <Alignment ss:Vertical=\"Bottom\" />" "\n"
"      <Borders />" "\n"
"      <Font />" "\n"
"      <Interior />" "\n"
"      <NumberFormat />" "\n"
"      <Protection />" "\n"
"    </Style>" "\n"
"  </Styles>" "\n";

    status = plain_write_ustr(p_ff_op_format, str);

    return(status);
}

_Check_return_
static STATUS
xls_xml_write_worksheet_begin(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    PC_USTR str = USTR_TEXT(
"  <Worksheet ss:Name=\"Sheet1\">" "\n");

    status = plain_write_ustr(p_ff_op_format, str);

    return(status);
}

_Check_return_
static STATUS
xls_xml_write_worksheet_options(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    PC_USTR str = (PC_USTR)
"    <WorksheetOptions xmlns=\"urn:schemas-microsoft-com:office:excel\">" "\n"
"      <Print>" "\n"
"        <ValidPrinterInfo />" "\n"
"        <HorizontalResolution>600</HorizontalResolution>" "\n"
"        <VerticalResolution>600</VerticalResolution>" "\n"
"      </Print>" "\n"
"      <Selected />" "\n"
"      <Panes>" "\n"
"        <Pane>" "\n"
"          <Number>3</Number>" "\n"
"          <ActiveRow>5</ActiveRow>" "\n"
"          <ActiveCol>1</ActiveCol>" "\n"
"        </Pane>" "\n"
"      </Panes>" "\n"
"      <ProtectObjects>False</ProtectObjects>" "\n"
"      <ProtectScenarios>False</ProtectScenarios>" "\n"
"    </WorksheetOptions>" "\n";

    status = plain_write_ustr(p_ff_op_format, str);

    return(status);
}

_Check_return_
static STATUS
xls_xml_write_worksheet_end(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    PC_USTR str = USTR_TEXT(
"  </Worksheet>" "\n");

    status = plain_write_ustr(p_ff_op_format, str);

    return(status);
}

_Check_return_
static STATUS
xls_xml_write_eof(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    PC_USTR str = USTR_TEXT(
"</Workbook>" "\n");

    status = plain_write_ustr(p_ff_op_format, str);

    return(status);
}

/******************************************************************************
*
* write out all the table information
*
******************************************************************************/

_Check_return_
static STATUS
xls_xml_write_table(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        OBJECT_DATA object_data;
        ROW on_row = MAX_ROW;

        status_return(xls_xml_output_table(p_ff_op_format,
            scan_block.docu_area.br.slr.col /*- scan_block.docu_area.tl.slr.col*/,
            scan_block.docu_area.br.slr.row /*- scan_block.save_docu_area.tl.slr.row*/));

        xml_level++;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            if(OBJECT_ID_NONE == object_data.object_id)
                continue;

            if(P_DATA_NONE == object_data.u.p_object)
                continue;

            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            if(on_row != object_data.data_ref.arg.slr.row)
            {   /* moved to a new row */
                if(MAX_ROW != on_row)
                {
                    xml_level--;

                    status_return(xml_output_tag(p_ff_op_format,
                        USTR_TEXT("</Row>" "\n")));
                }

                on_row = object_data.data_ref.arg.slr.row;

                status_return(xls_xml_output_row(p_ff_op_format, on_row));

                xml_level++;
            }

            switch(object_data.object_id)
            {
            case OBJECT_ID_TEXT:
                status = xls_xml_write_cell_text(p_docu, p_ff_op_format, &object_data);
                break;

            case OBJECT_ID_SS:
                status = xls_xml_write_cell_ss(p_docu, p_ff_op_format, &object_data);
                break;

            default:
                break;
            }

            status_break(status);
        }

        if(status_ok(status) && (MAX_ROW != on_row))
        {
            xml_level--;

            status = xml_output_tag(p_ff_op_format, USTR_TEXT("</Row>" "\n"));
        }
 
        if(status_ok(status))
        {
            xml_level--;

            status = xml_output_tag(p_ff_op_format, USTR_TEXT("</Table>" "\n"));
        }
   }

    return(status);
}

_Check_return_
extern STATUS
xls_save_xml(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status;

    limits_from_docu_area(p_docu, &g_s_col, &g_e_col, &g_s_row, &g_e_row, &p_ff_op_format->of_op_format.save_docu_area);

    if( status_ok(status = xls_xml_write_bof(p_ff_op_format)) &&

        status_ok(status = xls_xml_write_worksheet_begin(p_ff_op_format)) &&
        status_ok(status = xls_xml_write_table(p_docu, p_ff_op_format)) &&
        status_ok(status = xls_xml_write_worksheet_options(p_ff_op_format)) &&
        status_ok(status = xls_xml_write_worksheet_end(p_ff_op_format)) &&

        status_ok(status = xls_xml_write_eof(p_ff_op_format)) )
    { /*EMPTY*/ }

    return(status);
}

/* end of fs_xls_savex.c */
