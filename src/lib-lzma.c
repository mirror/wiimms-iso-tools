
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include "lib-lzma.h"
#include "lzma/LzmaEnc.h"
#include "lzma/LzmaDec.h"
#include "lzma/Lzma2Enc.h"
#include "lzma/Lzma2Dec.h"

/***********************************************
 **  LZMA SDK: http://www.7-zip.org/sdk.html  **
 ***********************************************/

//
///////////////////////////////////////////////////////////////////////////////
///////////////			LZMA helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetMessageLZMA
(
    int			err,		// error code
    ccp			unkown_error	// result for unkown error codes
)
{
    switch (err)
    {
	case SZ_OK:			return "OK";

	case SZ_ERROR_ARCHIVE:		return "Archive error";
	case SZ_ERROR_CRC:		return "CRC error";
	case SZ_ERROR_DATA:		return "Data error";
	case SZ_ERROR_FAIL:		return "Operation failed";
	case SZ_ERROR_INPUT_EOF:	return "Need more input";
	case SZ_ERROR_MEM:		return "Memory allocation error";
	case SZ_ERROR_NO_ARCHIVE:	return "No archive";
	case SZ_ERROR_OUTPUT_EOF:	return "Putput buffer overflow";
	case SZ_ERROR_PARAM:		return "Incorrect parameter";
	case SZ_ERROR_PROGRESS:		return "Some break from progress callback";
	case SZ_ERROR_READ:		return "Reading error";
	case SZ_ERROR_THREAD:		return "Errors in multithreading functions";
	case SZ_ERROR_UNSUPPORTED:	return "Unsupported properties";
	case SZ_ERROR_WRITE:		return "Writing error";
    }

    return unkown_error;
};

///////////////////////////////////////////////////////////////////////////////

int CalcCompressionLevelLZMA
(
    int			compr_level	// valid are 1..9 / 0: use default value
)
{
    return compr_level >= 1 && compr_level <= 9 ? compr_level : 9;
}

///////////////////////////////////////////////////////////////////////////////

static void * AllocLZMA ( void *p, size_t size )
{
    return malloc(size);
}

static void FreeLZMA ( void *p, void *ptr )
{
    free(ptr);
}

static ISzAlloc lzma_alloc = { AllocLZMA, FreeLZMA };

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    static LZMA write helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct sz_inbuf_t
{
    ISeqInStream	func;
    DataList_t		* data;
    u32			bytes_read;

} sz_inbuf_t;

//-----------------------------------------------------------------------------

static SRes sz_read_buf ( void *pp, void *buf, size_t *size )
{
    DASSERT(pp);
    DASSERT(size);
    sz_inbuf_t * ibuf = pp;
    noPRINT("sz_read_buf(%p,%p,%zx=%zu)\n",ibuf,buf,*size,*size);

    ibuf->bytes_read += *size = ReadDataList(ibuf->data,buf,*size);
    noPRINT("sz_read_buf() size = %u\n",*size);
    return SZ_OK;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct sz_outfile_t
{
    ISeqOutStream	func;
    File_t		* file;
    u32			bytes_written;

} sz_outfile_t;

//-----------------------------------------------------------------------------

static size_t sz_write_file ( void *pp, const void *data, size_t size )
{
    DASSERT(pp);
    sz_outfile_t * obuf = pp;
    noPRINT("sz_write_file(%p->%p,%p,%zx=%zu)\n",obuf,obuf->file,data,size,size);
    obuf->bytes_written += size;
    return WriteF(obuf->file,data,size) ? 0 : size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		  LZMA encoding (compression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_Open
(
    EncLZMA_t		* lzma,		// object, will be initialized
    ccp			error_object,	// object name for error messages
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_endmark	// true: write end marker at end of stream
)
{
    DASSERT(lzma);
    memset(lzma,0,sizeof(*lzma));
    lzma->error_object = error_object ? error_object : "?";


    //----- create handle

    lzma->handle = LzmaEnc_Create(&lzma_alloc);
    if (!lzma->handle)
	return ERROR0(ERR_LZMA,
		"Error while opening LZMA stream: %s\n-> LZMA error: %s\n",
		lzma->error_object, GetMessageLZMA(SZ_ERROR_MEM,"?") );


    //----- compression method and properties

    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    lzma->compr_level = CalcCompressionLevelLZMA(compr_level);
    props.level = lzma->compr_level;
    props.writeEndMark = write_endmark;

    // [2do] size optimization

    SRes res = LzmaEnc_SetProps(lzma->handle,&props);
    if ( res != SZ_OK )
    {
	EncLZMA_Close(lzma);
	return ERROR0(ERR_LZMA,
		"Error while setup LZMA properties: %s\n-> LZMA error: %s\n",
		lzma->error_object, GetMessageLZMA(res,"?") );
    }

    //----- store encoded properties

    DASSERT( sizeof(lzma->enc_props) >= LZMA_PROPS_SIZE );
    lzma->enc_props_len = sizeof(lzma->enc_props);
    res = LzmaEnc_WriteProperties(lzma->handle,lzma->enc_props,&lzma->enc_props_len);
    if ( res != SZ_OK )
    {
	EncLZMA_Close(lzma);
	return ERROR0(ERR_LZMA,
		"Error while writing LZMA properties: %s\n-> LZMA error: %s\n",
		lzma->error_object, GetMessageLZMA(res,"?") );
    }

    noPRINT("EncLZMA_Open() done: prop-size=%d\n", lzma->enc_props_len );

    return ERR_OK;
};

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_WriteDataToFile
(
    EncLZMA_t		* lzma,		// valid pointer, opend with EncLZMA_Open()
    File_t		* file,		// destination file, write to current offset
    bool		write_props,	// true: write encoding properties
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DataArea_t area[2];
    memset(&area,0,sizeof(area));
    area->data = data;
    area->size = data_size;

    DataList_t list;
    SetupDataList(&list,area);

    return EncLZMA_WriteList2File(lzma,file,write_props,&list,bytes_written);
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_WriteList2File
(
    EncLZMA_t		* lzma,		// valid pointer, opend with EncLZMA_Open()
    File_t		* file,		// destination file, write to current offset
    bool		write_props,	// true: write encoding properties
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DASSERT(lzma);
    DASSERT(lzma->handle);
    DASSERT(file);

    if (write_props)
    {
	enumError err = WriteF(file,lzma->enc_props,lzma->enc_props_len);
	if (err)
	    return err;
    }

    sz_inbuf_t inbuf;
    inbuf.func.Read = sz_read_buf;
    inbuf.data = data_list;
    inbuf.bytes_read = 0;

    sz_outfile_t outfile;
    outfile.func.Write = sz_write_file;
    outfile.file = file;
    outfile.bytes_written = write_props ? lzma->enc_props_len : 0;

    SRes res = LzmaEnc_Encode( lzma->handle,
				(ISeqOutStream*)&outfile,
				(ISeqInStream*)&inbuf,
				0, &lzma_alloc, &lzma_alloc );
    if ( res != SZ_OK )
    {
	EncLZMA_Close(lzma);
	return ERROR0(ERR_LZMA,
		"Error while writing LZMA stream: %s\n-> LZMA error: %s\n",
		lzma->error_object, GetMessageLZMA(res,"?") );
    }
    
    // count only uncomressed size
    file->bytes_written += inbuf.bytes_read - outfile.bytes_written;

    if (bytes_written)
	*bytes_written = outfile.bytes_written;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_Close
(
    EncLZMA_t		* lzma		// valid pointer
)
{
    DASSERT(lzma);
    if ( lzma->handle )
    {
	LzmaEnc_Destroy(lzma->handle,&lzma_alloc,&lzma_alloc);
	lzma->handle = 0;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_Data2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    File_t		* file,		// destination file, write to current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DataArea_t area[2];
    memset(&area,0,sizeof(area));
    area->data = data;
    area->size = data_size;

    DataList_t list;
    SetupDataList(&list,area);

    return EncLZMA_List2File(lzma,file,compr_level,
				write_props,write_endmark,&list,bytes_written);
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_List2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    File_t		* file,		// destination file, write to current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DASSERT(file);

    EncLZMA_t internal_lzma;
    if (!lzma)
	lzma = &internal_lzma;

    enumError err = EncLZMA_Open(lzma,file->fname,compr_level,write_endmark);
    if (err)
	return err;

    err = EncLZMA_WriteList2File(lzma,file,write_props,data_list,bytes_written);
    if (err)
	return err;

    return EncLZMA_Close(lzma);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		LZMA decoding (decompression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DecLZMA_File2Buf // open + read + close lzma stream
(
    File_t		* file,		// source file, read from current offset
    size_t		read_count,	// not NULL: max bytes to read from file
    void		* buf,		// destination buffer
    size_t		buf_size,	// size of destination buffer
    u32			* bytes_written,// not NULL: store bytes written to buf
    u8			* enc_props	// Encoding properties
					// If NULL: read it from file
)
{
    DASSERT(file);
    DASSERT(buf);

    u8 prop_buf[LZMA_PROPS_SIZE];
    if (!enc_props)
    {
	const enumError err = ReadF(file,prop_buf,sizeof(prop_buf));
	if (err)
	    return err;
	enc_props = prop_buf;
    }

    CLzmaDec lzma;
    LzmaDec_Construct(&lzma);
    SRes res = LzmaDec_Allocate(&lzma,enc_props,LZMA_PROPS_SIZE,&lzma_alloc);
    if ( res != SZ_OK )
	return ERROR0(ERR_LZMA,
		"Error while setup LZMA properties: %s\n-> LZMA error: %s\n",
		file->fname, GetMessageLZMA(res,"?") );

    enumError err = ERR_OK;
    u8 in_buf[0x10000];
    size_t in_buf_len = 0;

    const int read_behind_eof = file->read_behind_eof;
    if ( !read_count && file->seek_allowed && file->st.st_size )
	read_count = file->st.st_size - file->cur_off;
    const bool have_max_read = read_count > 0;
    if (!have_max_read)
	file->read_behind_eof = 2;

    u8 * dest = buf;
    u32 written = 0;

    LzmaDec_Init(&lzma);
    for(;;)
    {
	//--- fill input buffer

	ELzmaFinishMode finish = LZMA_FINISH_ANY;

	DASSERT( in_buf_len >= 0 && in_buf_len < sizeof(in_buf) );
	size_t read_size = sizeof(in_buf) - in_buf_len;
	if ( have_max_read && read_size > read_count )
	{
	    read_size = read_count;
	    finish = LZMA_FINISH_END;
	}
	noPRINT("READ off=%llx, size=%x,%u, to=%u\n",
		file->cur_off,read_size,read_size,in_buf_len);
	err = ReadF(file,in_buf+in_buf_len,read_size);
	if (err)
	    return err;
	in_buf_len	 += read_size;
	read_count	 -= read_size;
	file->bytes_read -= read_size; // count only decompressed data

	if (!in_buf_len)
	    break;


	//--- decode

	size_t in_len  = in_buf_len;
	size_t out_len = buf_size;
	ELzmaStatus status;

	res = LzmaDec_DecodeToBuf(&lzma,dest,&out_len,in_buf,&in_len,finish,&status);
	noPRINT("DECODED, res=%s, stat=%d, in=%u/%u out=%u/%u\n",
		GetMessageLZMA(res,"?"), status,
		in_len, in_buf_len, out_len, buf_size );

	if ( res == SZ_OK && !in_len && !out_len && status != LZMA_STATUS_FINISHED_WITH_MARK )
	    res = SZ_ERROR_DATA;

	if ( res != SZ_OK )
	    return ERROR0(ERR_LZMA,
		"Error while reading LZMA stream: %s\n-> LZMA error: %s\n",
		file->fname, GetMessageLZMA(res,"?") );

	written		 += out_len;
	dest		 += out_len;
	buf_size	 -= out_len;
	file->bytes_read += out_len; // count only decompressed data

	if ( in_len < in_buf_len )
	{
	    in_buf_len -= in_len;
	    memmove(in_buf,in_buf+in_len,in_buf_len);
	}
	else
	    in_buf_len = 0;

	if ( status == LZMA_STATUS_FINISHED_WITH_MARK )
	    break;
    }

    file->read_behind_eof = read_behind_eof;
    LzmaDec_Free(&lzma,&lzma_alloc);

    if (bytes_written)
	*bytes_written = written;
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		  LZMA2 encoding (compression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_Open
(
    EncLZMA_t		* lzma,		// object, will be initialized
    ccp			error_object,	// object name for error messages
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_endmark	// true: write end marker at end of stream
)
{
    DASSERT(lzma);
    memset(lzma,0,sizeof(*lzma));
    lzma->error_object = error_object ? error_object : "?";


    //----- create handle

    lzma->handle = Lzma2Enc_Create(&lzma_alloc,&lzma_alloc);
    if (!lzma->handle)
	return ERROR0(ERR_LZMA,
		"Error while opening LZMA2 stream: %s\n-> LZMA2 error: %s\n",
		lzma->error_object, GetMessageLZMA(SZ_ERROR_MEM,"?") );


    //----- compression method and properties

    CLzma2EncProps props;
    Lzma2EncProps_Init(&props);
    lzma->compr_level = CalcCompressionLevelLZMA(compr_level);
    props.lzmaProps.level = lzma->compr_level;
    props.lzmaProps.writeEndMark = write_endmark;

    // [2do] size optimization

    SRes res = Lzma2Enc_SetProps(lzma->handle,&props);
    if ( res != SZ_OK )
    {
	EncLZMA2_Close(lzma);
	return ERROR0(ERR_LZMA,
		"Error while setup LZMA2 properties: %s\n-> LZMA2 error: %s\n",
		lzma->error_object, GetMessageLZMA(res,"?") );
    }

    //----- store encoded properties

    DASSERT( sizeof(lzma->enc_props) >= LZMA_PROPS_SIZE );
    lzma->enc_props_len = sizeof(lzma->enc_props);
    lzma->enc_props[0] = Lzma2Enc_WriteProperties(lzma->handle);
    lzma->enc_props_len = 1;

    noPRINT("EncLZMA2_Open() done: prop-size=%d, byte=%02x\n",
		lzma->enc_props_len, lzma->enc_props[0] );

    return ERR_OK;
};

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_WriteDataToFile
(
    EncLZMA_t		* lzma,		// valid pointer, opend with EncLZMA2_Open()
    File_t		* file,		// destination file, write to current offset
    bool		write_props,	// true: write encoding properties
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DataArea_t area[2];
    memset(&area,0,sizeof(area));
    area->data = data;
    area->size = data_size;

    DataList_t list;
    SetupDataList(&list,area);

    return EncLZMA2_WriteList2File(lzma,file,write_props,&list,bytes_written);
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_WriteList2File
(
    EncLZMA_t		* lzma,		// valid pointer, opend with EncLZMA2_Open()
    File_t		* file,		// destination file, write to current offset
    bool		write_props,	// true: write encoding properties
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DASSERT(lzma);
    DASSERT(lzma->handle);
    DASSERT(file);

    if (write_props)
    {
	enumError err = WriteF(file,lzma->enc_props,lzma->enc_props_len);
	if (err)
	    return err;
    }

    sz_inbuf_t inbuf;
    inbuf.func.Read = sz_read_buf;
    inbuf.data = data_list;
    inbuf.bytes_read = 0;

    sz_outfile_t outfile;
    outfile.func.Write = sz_write_file;
    outfile.file = file;
    outfile.bytes_written = write_props ? lzma->enc_props_len : 0;

    SRes res = Lzma2Enc_Encode( lzma->handle,
				(ISeqOutStream*)&outfile,
				(ISeqInStream*)&inbuf, 0 );
    if ( res != SZ_OK )
    {
	EncLZMA2_Close(lzma);
	return ERROR0(ERR_LZMA,
		"Error while writing LZMA2 stream: %s\n-> LZMA2 error: %s\n",
		lzma->error_object, GetMessageLZMA(res,"?") );
    }

    // count only uncomressed size
    file->bytes_written += inbuf.bytes_read - outfile.bytes_written;

    if (bytes_written)
	*bytes_written = outfile.bytes_written;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_Close
(
    EncLZMA_t		* lzma		// valid pointer
)
{
    DASSERT(lzma);
    if ( lzma->handle )
    {
	Lzma2Enc_Destroy(lzma->handle);
	lzma->handle = 0;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_Data2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    File_t		* file,		// destination file, write to current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DataArea_t area[2];
    memset(&area,0,sizeof(area));
    area->data = data;
    area->size = data_size;

    DataList_t list;
    SetupDataList(&list,area);

    return EncLZMA2_List2File(lzma,file,compr_level,
				write_props,write_endmark,&list,bytes_written);
}

///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_List2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    File_t		* file,		// destination file, write to current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
)
{
    DASSERT(file);

    EncLZMA_t internal_lzma;
    if (!lzma)
	lzma = &internal_lzma;

    enumError err = EncLZMA2_Open(lzma,file->fname,compr_level,write_endmark);
    if (err)
	return err;

    err = EncLZMA2_WriteList2File(lzma,file,write_props,data_list,bytes_written);
    if (err)
	return err;

    return EncLZMA2_Close(lzma);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		LZMA2 decoding (decompression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DecLZMA2_File2Buf // open + read + close lzma stream
(
    File_t		* file,		// source file, read from current offset
    size_t		read_count,	// not NULL: max bytes to read from file
    void		* buf,		// destination buffer
    size_t		buf_size,	// size of destination buffer
    u32			* bytes_written,// not NULL: store bytes written to buf
    u8			* enc_props	// Encoding properties
					// If NULL: read it from file
)
{
    DASSERT(file);
    DASSERT(buf);

    u8 prop_buf[1];
    if (!enc_props)
    {
	const enumError err = ReadF(file,prop_buf,sizeof(prop_buf));
	if (err)
	    return err;
	enc_props = prop_buf;
    }

    CLzma2Dec lzma;
    Lzma2Dec_Construct(&lzma);
    SRes res = Lzma2Dec_Allocate(&lzma,*enc_props,&lzma_alloc);
    if ( res != SZ_OK )
	return ERROR0(ERR_LZMA,
		"Error while setup LZMA properties: %s\n-> LZMA error: %s\n",
		file->fname, GetMessageLZMA(res,"?") );

    enumError err = ERR_OK;
    u8 in_buf[0x10000];
    size_t in_buf_len = 0;

    const int read_behind_eof = file->read_behind_eof;
    if ( !read_count && file->seek_allowed && file->st.st_size )
	read_count = file->st.st_size - file->cur_off;
    const bool have_max_read = read_count > 0;
    if (!have_max_read)
	file->read_behind_eof = 2;

    u8 * dest = buf;
    u32 written = 0;

    Lzma2Dec_Init(&lzma);
    for(;;)
    {
	//--- fill input buffer

	ELzmaFinishMode finish = LZMA_FINISH_ANY;

	DASSERT( in_buf_len >= 0 && in_buf_len < sizeof(in_buf) );
	size_t read_size = sizeof(in_buf) - in_buf_len;
	if ( have_max_read && read_size > read_count )
	{
	    read_size = read_count;
	    finish = LZMA_FINISH_END;
	}
	noPRINT("READ off=%llx, size=%x,%u, to=%u\n",
		file->cur_off,read_size,read_size,in_buf_len);
	err = ReadF(file,in_buf+in_buf_len,read_size);
	if (err)
	    return err;
	in_buf_len	 += read_size;
	read_count	 -= read_size;
	file->bytes_read -= read_size; // count only decompressed data

	if (!in_buf_len)
	    break;


	//--- decode

	size_t in_len  = in_buf_len;
	size_t out_len = buf_size;
	ELzmaStatus status;

	res = Lzma2Dec_DecodeToBuf(&lzma,dest,&out_len,in_buf,&in_len,finish,&status);
	noPRINT("DECODED, res=%s, stat=%d, in=%u/%u out=%u/%u\n",
		GetMessageLZMA(res,"?"), status,
		in_len, in_buf_len, out_len, buf_size );

	if ( res == SZ_OK && !in_len && !out_len && status != LZMA_STATUS_FINISHED_WITH_MARK )
	    res = SZ_ERROR_DATA;

	if ( res != SZ_OK )
	    return ERROR0(ERR_LZMA,
		"Error while reading LZMA stream: %s\n-> LZMA error: %s\n",
		file->fname, GetMessageLZMA(res,"?") );

	written		 += out_len;
	dest		 += out_len;
	buf_size	 -= out_len;
	file->bytes_read += out_len; // count only decompressed data

	if ( in_len < in_buf_len )
	{
	    in_buf_len -= in_len;
	    memmove(in_buf,in_buf+in_len,in_buf_len);
	}
	else
	    in_buf_len = 0;

	if ( status == LZMA_STATUS_FINISHED_WITH_MARK )
	    break;
    }

    file->read_behind_eof = read_behind_eof;
    Lzma2Dec_Free(&lzma,&lzma_alloc);

    if (bytes_written)
	*bytes_written = written;
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

