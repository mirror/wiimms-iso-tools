
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

#include <bzlib.h>

#include "debug.h"
#include "iso-interface.h"

///////////////////////////////////////////////////////////////////////////////

/************************************************************************
 **  BZIP2 support: http://www.bzip.org/1.0.5/bzip2-manual-1.0.5.html  **
 ************************************************************************/

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    manage WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetWIA
(
    wia_controller_t	* wia		// NULL or valid pointer
)
{
    if (wia)
    {
	free(wia->pte);
	free(wia->part);
	free(wia->part_info);
	wd_reset_patch(&wia->memmap);

	memset(wia,0,sizeof(*wia));
    }
}

///////////////////////////////////////////////////////////////////////////////

char * PrintVersionWIA
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			version		// version number to print
)
{
    enum
    {
	SBUF_COUNT = 4,
	SBUF_SIZE  = 20
    };

    static int  sbuf_index = 0;
    static char sbuf[SBUF_COUNT][SBUF_SIZE+1];

    if (!buf)
    {
	// use static buffer
	buf = sbuf[sbuf_index];
	buf_size = SBUF_SIZE;
	sbuf_index = ( sbuf_index + 1 ) % SBUF_COUNT;
    }

    version = htonl(version); // we need big endian here
    const u8 * v = (const u8 *)&version;

    if (v[2])
    {
	if ( !v[3] || v[3] == 0xff )
	    snprintf(buf,buf_size,"%u.%02x.%02x",v[0],v[1],v[2]);
	else
	    snprintf(buf,buf_size,"%u.%02x.%02x.beta%u",v[0],v[1],v[2],v[3]);
    }
    else
    {
	if ( !v[3] || v[3] == 0xff )
	    snprintf(buf,buf_size,"%u.%02x",v[0],v[1]);
	else
	    snprintf(buf,buf_size,"%u.%02x.beta%u",v[0],v[1],v[3]);
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetCompressionNameWIA
(
    wia_compression_t	compr,		// compression mode
    ccp			invalid_result	// return value if 'compr' is invalid
)
{
    static ccp tab[] =
    {
	"uncompressed",
	"bzip2",
    };
    
    return (u32)compr < WIA_COMPR__N ? tab[compr] : invalid_result;
}

///////////////////////////////////////////////////////////////////////////////

bool IsWIA
(
    const void		* data,		// data to check
    size_t		data_size,	// size of data
    void		* id6_result	// not NULL: store ID6 (6 bytes without null term)
)
{
    bool is_wia = false;
    const wia_file_head_t * fhead = data;
    if ( data_size >= sizeof(wia_file_head_t) )
    {
	if (!memcmp(fhead->magic,WIA_MAGIC,sizeof(fhead->magic)))
	{
	    sha1_hash hash;
	    SHA1( (u8*)fhead, sizeof(*fhead)-sizeof(fhead->file_head_hash), hash );
	    is_wia = !memcmp(hash,fhead->file_head_hash,sizeof(hash));
	    //HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);
	    //HEXDUMP(0,0,0,-WII_HASH_SIZE,fhead->file_head_hash,WII_HASH_SIZE);
	}
    }

    if (id6_result)
    {
	memset(id6_result,0,6);
	if (is_wia)
	{
	    const wia_disc_t * disc = (const wia_disc_t*)(fhead+1);
	    if ( data_size >= (ccp)&disc->dhead.disc_id - (ccp)data + 6 )
		memcpy(id6_result,&disc->dhead.disc_id,6);
	}
    }
    
    return is_wia;
}

///////////////////////////////////////////////////////////////////////////////

void SetupMemMap
(
    wia_controller_t	* wia		// valid pointer
)
{
    DASSERT(wia);
    wd_patch_t * mm = &wia->memmap;
    wd_reset_patch(mm);

    wd_patch_item_t * it;
    wia_disc_t * disc = &wia->disc;
    
    //----- disc header

    it = wd_insert_patch(mm,WD_PAT_DATA,0,sizeof(disc->dhead));
    DASSERT(it);
    it->data = &disc->dhead;
    snprintf(it->info,sizeof(it->info),
	"Disc header, id=%s", wd_print_id(&disc->dhead.disc_id,6,0) );

    //----- disc data

    it = wd_insert_patch(mm,WD_PAT_DATA,
		sizeof(disc->dhead),sizeof(wia->disc_data));
    DASSERT(it);
    it->data = wia->disc_data;
    snprintf(it->info,sizeof(it->info),"Disc data");


    //----- partitions

    if (wia->part_info)
    {
	int ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wia_part_t *part = wia->part + ip;
	    wd_part_header_t *ph
		= (wd_part_header_t*)( wia->part_info + part->ticket_off );

	    //--- ticket

	    const u64 base_off = part->part_off;
	    it = wd_insert_patch(mm,WD_PAT_DATA,
			base_off, sizeof(wd_part_header_t) );
	    DASSERT(it);
	    it->data = ph;
	    snprintf(it->info,sizeof(it->info),
			"P.%u.%u, ticket, id=%s, ckey=%u",
			part->ptab_index, part->ptab_part_index,
			wd_print_id(ph->ticket.title_id+4,4,0),
			ph->ticket.common_key_index );

	    //--- tmd

	    if ( ph->tmd_off4 )
	    {
		wd_tmd_t * tmd = (wd_tmd_t*)( wia->part_info + part->tmd_off );
		it = wd_insert_patch(mm,WD_PAT_DATA,
			    base_off + ((u64)ntohl(ph->tmd_off4)<<2),
			    ntohl(ph->tmd_size));
		DASSERT(it);
		it->data = tmd;
		snprintf(it->info,sizeof(it->info),
			"P.%u.%u, tmd, id=%s",
			part->ptab_index, part->ptab_part_index,
			wd_print_id(tmd->title_id+4,4,0) );
	    } 

	    //--- cert

	    if ( ph->cert_off4 )
	    {
		u8 * cert = wia->part_info + part->cert_off;
		it = wd_insert_patch(mm,WD_PAT_DATA,
			    base_off + ((u64)ntohl(ph->cert_off4)<<2),
			    ntohl(ph->cert_size));
		DASSERT(it);
		it->data = cert;
		snprintf(it->info,sizeof(it->info),
			"P.%u.%u, cert",
			part->ptab_index, part->ptab_part_index );
	    } 

	    //--- h3

	    if ( ph->h3_off4 )
	    {
		u8 * h3 = wia->part_info + part->h3_off;
		it = wd_insert_patch(mm,WD_PAT_DATA,
			    base_off + ((u64)ntohl(ph->h3_off4)<<2),
			    WII_H3_SIZE );
		DASSERT(it);
		it->data = h3;
		snprintf(it->info,sizeof(it->info),
			"P.%u.%u, h3",
			part->ptab_index, part->ptab_part_index );
	    } 

	    //--- data

	    it = wd_insert_patch(mm,WD_PAT_PART_DATA,
			part->first_sector * (u64)WII_SECTOR_SIZE,
			part->n_sectors * (u64)WII_SECTOR_SIZE );
	    DASSERT(it);
	    it->part_index = ip;
	    snprintf(it->info,sizeof(it->info),
			"P.%u.%u, %u groups, %u sectors",
			part->ptab_index, part->ptab_part_index,
			part->n_groups, part->n_sectors );
	}
    }


    //----- logging

    if ( logging > 0 )
    {
	printf("\nWIA memory map:\n\n");
	wd_dump_patch(stdout,3,mm);
	putchar('\n');
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ReadWIA() + read helpers	///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError read_compressed_data
(
    SuperFile_t		* sf,		// source file
    u64			file_offset,	// file offset
    u32			file_data_size,	// expected file data size
    void		* inbuf,	// valid pointer to data
    u32			inbuf_size,	// size of data to write
    u32			* read_count	// not NULL: store read data count
)
{   
    DASSERT( sf );
    DASSERT( sf->wia );
    DASSERT( inbuf );
    DASSERT( inbuf_size );

    switch(sf->wia->disc.compression)
    {
	case WIA_COMPR_BZIP2:
	{ 
	    ASSERT(sf->f.fp);
	    enumError err = SeekF(&sf->f,file_offset);
	    if (err)
		return err;

	    int bzerror;
	    BZFILE *bz = BZ2_bzReadOpen(&bzerror,sf->f.fp,0,0,0,0);
	    if ( !bz || bzerror != BZ_OK )
	    {
		if (bz)
		    BZ2_bzReadClose(0,bz);
		return ERROR0(ERR_BZIP2,
				"Error while opening bzip2 stream: %s\n",sf->f.fname);
	    }

	    int bytes_read = BZ2_bzRead(&bzerror,bz,inbuf,inbuf_size);
	    PRINT("BZREAD, num=%x, datasize=%x, err=%d\n",bytes_read,file_data_size,bzerror);
	    if ( bzerror != BZ_STREAM_END )
		return ERROR0(ERR_BZIP2,
				"Error while reading bzip2 stream: %s\n",sf->f.fname);

	    BZ2_bzReadClose(&bzerror,bz);
	    if ( bzerror != BZ_OK )
		return ERROR0(ERR_BZIP2,
				"Error while closing bzip2 stream: %s\n",sf->f.fname);

	    if (read_count)
		*read_count = bytes_read;
	}
	break;

	default:
	{
	    if ( file_data_size <= sizeof(sha1_hash) || file_data_size > inbuf_size )
		return ERROR0(ERR_WIA_INVALID,
		    "Invalid WIA data segment: %s\n",sf->f.fname);

	    file_data_size -= sizeof(sha1_hash);
	    enumError err = ReadAtF( &sf->f, file_offset, inbuf, file_data_size );
	    if (err)
		return err;

	    sha1_hash hash, file_hash;
	    err = ReadF( &sf->f, &file_hash, WII_HASH_SIZE );
	    if (err)
		return err;
	    
	    SHA1(inbuf,file_data_size,hash);
	    if (memcmp(hash,file_hash,WII_HASH_SIZE))
	    {
		HEXDUMP16(0,0,inbuf,16);
		HEXDUMP(0,0,0,-WII_HASH_SIZE,file_hash,WII_HASH_SIZE);
		HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);
		return ERROR0(ERR_WIA_INVALID,
		    "SHA1 check for WIA data segment failed: %s\n",sf->f.fname);
	    }

	    if (read_count)
		*read_count = file_data_size;
	}
	break;
    }
    
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError expand_segments
(
    SuperFile_t		* sf,		// source file
    const wia_data_segment_t
			* seg,		// source segment pointer
    void		* seg_end,	// end of segment space
    void		* dest_ptr,	// pointer to destination
    u32			dest_size	// size of destination
)
{
    DASSERT( seg );
    DASSERT( seg_end );
    DASSERT( dest_ptr );
    DASSERT( dest_size >= 8 );
    DASSERT( !(dest_size&3) );

    u8 * dest = dest_ptr;

    for(;;)
    {
	const u32 offset = ntohl(seg->offset);
	const u32 size   = ntohl(seg->size);
	if (!size)
	    break;

	noPRINT("SEG: %p: %x+%x\n",seg,offset,size);

	if ( (u8*)seg >= (u8*)seg_end || offset + size > dest_size )
	{
	    PRINT("seg=%p..%p, off=%x, size=%x, end=%x/%x\n",
		seg, seg_end, offset, size, offset + size, dest_size );
	    return ERROR0(ERR_WIA_INVALID,
		"Invalid WIA data segment failed: %s\n",sf->f.fname);
	}

	memcpy( dest + offset, seg->data, size );
	seg = (wia_data_segment_t*)( seg->data + size );
    }
    
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError read_part_data
(
    struct SuperFile_t	* sf,		// source file
    u32			part_index,	// partition index
    u32			group		// group index
    
)
{
    DASSERT(sf);
    DASSERT(sf->wia);
    wia_controller_t * wia = sf->wia;
    enumError err = ERR_OK;

    if ( wia->gdata_part_index == part_index && wia->gdata_group == group )
	return ERR_OK;

    wia->gdata_part_index	= part_index;
    wia->gdata_group		= group;
    memset(wia->gdata,0,sizeof(wia->gdata));


    //----- get group info

    wia_part_t  * part	= wia->part + wia->gdata_part_index;
    wia_group_t * grp	= ( wia_group_t *) ( wia->part_info + part->group_off )
			+ group;
    ASSERT( (u8*)grp + sizeof(*grp) <= wia->part_info + wia->part_info_size );

    u64 data_off  = (u64)ntohl(grp->data_off4) << 2;
    u32 data_size = ntohl(grp->data_size);

    PRINT(" GRP=%5u[%zx], off=%llx, datasize=%x=%u\n",
		wia->gdata_group, (u8*)grp - wia->part_info,
		data_off, data_size, data_size );

    if (!data_size)
	return ERR_OK;

    if ( data_off + data_size > wia->fhead.file_size )
	return ERROR0(ERR_WIA_INVALID,
		"Invalid WIA data offset: %llx (max=%llx): %s\n",
		data_off + data_size, wia->fhead.file_size, sf->f.fname );

    if ( data_size > sizeof(wia->iobuf) )
	return ERROR0(ERR_WIA_INVALID,
		"Invalid WIA data size: %x (max=%x): %s\n",
		data_size, sizeof(wia->iobuf), sf->f.fname);


    //----- load and decompress data

    err = read_compressed_data( sf, data_off, data_size,
				wia->iobuf, sizeof(wia->iobuf), &data_size );
    if (err)
	return err;


    //----- process data segments

    wia_data_t * data = (wia_data_t*)wia->iobuf;
    wia_exception_t * except = data->exception;

    wia_data_segment_t * seg
	= (wia_data_segment_t*)( wia->iobuf + ntohl(data->seg_offset) );

    err = expand_segments( sf, seg, wia->iobuf + data_size,
				wia->gdata, sizeof(wia->gdata) );
    if (err)
	return err;
    

    //----- process hash and exceptions

    u8 hashtab[WII_GROUP_HASH_SIZE+WII_HASH_SIZE]; // + reserve top avoid buf overflow
    memset(hashtab,0,sizeof(hashtab));
    wd_calc_group_hashes(wia->gdata,hashtab,0,0);

    u32 n_exceptions = ntohs(data->n_exceptions);
    while ( n_exceptions-- > 0 )
    {
	noPRINT("EXCEPT: %x\n",ntohs(except->offset));
	DASSERT( ntohs(except->offset) + WII_HASH_SIZE <= sizeof(hashtab) );
	memcpy( hashtab + ntohs(except->offset), except->hash, WII_HASH_SIZE );
	except++;
    }


    //----- encrpyt and join data

    if ( wia->encrypt )
    {
	aes_key_t akey;
	wd_aes_set_key(&akey,part->part_key);
	wd_encrypt_sectors(0,&akey,wia->gdata,hashtab,wia->gdata,WII_GROUP_SECTORS);
    }
    else
	wd_join_sectors(wia->gdata,hashtab,wia->gdata,WII_GROUP_SECTORS);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadWIA 
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* buf,		// destination buffer
    size_t		count		// number of bytes to read
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# ReadWIA()",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)off, (u64)off+count, count, "" );

    memset(buf,0,count);

    wia_controller_t * wia = sf->wia;
    if (!wia->is_valid)
	return ERR_WIA_INVALID;

    const u64 off2 = off + count;
    const wd_patch_item_t * item = wia->memmap.item;
    const wd_patch_item_t * item_end = item + wia->memmap.used;

    for ( ; item < item_end && item->offset < off2; item++ )
    {
	const u64 end = item->offset + item->size;
	noTRACE("> off=%llx..%llx, item=%llx..%llx\n", (u64)off, off2, item->offset, end );
	if ( item->offset < off2 && end > off )
	{
	    u64 overlap1 = item->offset > off ? item->offset : off;
	    const u64 overlap2 = end < off2 ? end : off2;
	    noTRACE(" -> %llx .. %llx\n",overlap1,overlap2);
	    sf->f.bytes_read += overlap2 - overlap1;

	    switch (item->mode)
	    {
	      case WD_PAT_DATA:
		noPRINT(" > READ DISC: %9llx .. %9llx\n",overlap1,overlap2);
		memcpy(	(char*)buf + (overlap1-off),
			(u8*)item->data + (overlap1-item->offset),
			overlap2 - overlap1 );
		break;

	      case WD_PAT_PART_DATA:
		while ( overlap1 < overlap2 )
		{
		    ASSERT( item->part_index < wia->disc.n_part );
		    ASSERT( wia->part );
		    wia_part_t * part = wia->part + item->part_index;
		    u32 sector = overlap1 / WII_SECTOR_SIZE - part->first_sector;
		    const u32 group = sector / WII_GROUP_SECTORS;
		    if ( item->part_index != wia->gdata_part_index || group != wia->gdata_group )
		    {
			const enumError err = read_part_data(sf,item->part_index,group);
			if (err)
			    return err;
		    }
		    
		    sector -= group * WII_GROUP_SECTORS;
		    const u64 group_off1 = group * (u64)WII_GROUP_SIZE
					 + part->first_sector * (u64)WII_SECTOR_SIZE;
		    u64 group_off2 = group_off1 + WII_GROUP_SIZE;
		    if ( group_off2 > overlap2 )
			group_off2 = overlap2;

		    noPRINT(" > READ P%u/%u.%u: %llx .. %llx -> %llx + %llx\n",
				item->part_index, group, sector,
				overlap1, group_off2,
				overlap1 - group_off1, group_off2 - overlap1 );
		    memcpy(	(char*)buf + (overlap1-off),
				wia->gdata + (overlap1-group_off1),
				group_off2 - overlap1 );
		    overlap1 = group_off2;
		}
		break;

	      default:
		ASSERT(0);
	    }
	}
    }
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup read WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadWIA 
(
    struct SuperFile_t	* sf	// file to setup
)
{
    PRINT("#W# SetupReadWIA(%p) wc=%p wbfs=%p v=%s/%s\n",
		sf, sf->wc, sf->wbfs,
		PrintVersionWIA(0,0,WIA_VERSION_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION) );
    ASSERT(sf);

    if (sf->wia)
	return ERROR0(ERR_INTERNAL,0);

    CleanSF(sf);
    OpenStreamFile(&sf->f);


    //----- setup controller

    wia_controller_t * wia = calloc(1,sizeof(*wia));
    if (!wia)
	return OUT_OF_MEMORY;
    sf->wia = wia;
    wia->gdata_group = ~(u32)0;
    wia->encrypt = encoding & ENCODE_ENCRYPT || !( encoding & ENCODE_DECRYPT );


    //----- read and check file header
    
    wia_file_head_t *fhead = &wia->fhead;
    enumError err = ReadAtF(&sf->f,0,fhead,sizeof(*fhead));
    if (err)
	return err;

    const bool is_wia = IsWIA(fhead,sizeof(*fhead),0);
    wia_ntoh_file_head(fhead,fhead);
    if ( !is_wia || fhead->disc_size > sizeof(wia->iobuf) )
	return ERROR0(ERR_WIA_INVALID,"Invalid file header: %s\n",sf->f.fname);

    if ( WIA_VERSION < fhead->version_compatible
	|| fhead->version < WIA_VERSION_COMPATIBLE )
    {
	if ( WIA_VERSION_COMPATIBLE < WIA_VERSION )
	    return ERROR0(ERR_WIA_INVALID,
		"Not supported WIA version %s (compatible %s .. %s): %s\n",
		PrintVersionWIA(0,0,fhead->version),
		PrintVersionWIA(0,0,WIA_VERSION_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION),
		sf->f.fname );
	else
	    return ERROR0(ERR_WIA_INVALID,
		"Not supported WIA version %s (%s expected): %s\n",
		PrintVersionWIA(0,0,fhead->version),
		PrintVersionWIA(0,0,WIA_VERSION),
		sf->f.fname );
    }


    //----- check file size

    if ( sf->f.st.st_size < fhead->file_size )
	SetupSplitFile(&sf->f,OFT_WIA,0);

    if ( sf->f.st.st_size != fhead->file_size )
	return ERROR0(ERR_WIA_INVALID,
		"Wrong file size %llu (%llu expected): %s\n",
		(u64)sf->f.st.st_size, fhead->file_size, sf->f.fname );


    //----- read and check disc info

    DASSERT( fhead->disc_size   <= sizeof(wia->iobuf) );
    DASSERT( sizeof(wia_disc_t) <= sizeof(wia->iobuf) );

    memset(wia->iobuf,0,sizeof(wia_disc_t));
    ReadAtF(&sf->f,sizeof(*fhead),wia->iobuf,fhead->disc_size);
    if (err)
	return err;

    sha1_hash hash;
    SHA1(wia->iobuf,fhead->disc_size,hash);
    if (memcmp(hash,fhead->disc_hash,sizeof(hash)))
	return ERROR0(ERR_WIA_INVALID,
	    "Hash error for disc area: %s\n",sf->f.fname);

    wia_disc_t *disc = &wia->disc;
    wia_ntoh_disc(disc,(wia_disc_t*)wia->iobuf);


    //----- read and check disc data

    PRINT("DISC DATA: %llx + %x\n", disc->disc_data_off, disc->disc_data_size );
    if ( disc->disc_data_off && disc->disc_data_size )
    {
	err = read_compressed_data(sf, disc->disc_data_off, disc->disc_data_size,
					wia->iobuf, sizeof(wia->iobuf), 0 );
	if (err)
	    return err;

	err = expand_segments( sf, (wia_data_segment_t*)wia->iobuf,
				wia->iobuf + sizeof(wia->iobuf),
				wia->disc_data, sizeof(wia->disc_data) );
    	if (err)
	    return err;
    }


    //----- read and check partition header

    const u32 load_part_size = disc->part_t_size * disc->n_part;
    if ( load_part_size > sizeof(wia->iobuf) )
	return ERROR0(ERR_WIA_INVALID,
	    "Total partition header size to large: %s\n",sf->f.fname);

    ReadAtF(&sf->f,disc->part_off,wia->iobuf,load_part_size);
    if (err)
	return err;

    SHA1(wia->iobuf,load_part_size,hash);
    if (memcmp(hash,disc->part_hash,sizeof(hash)))
	return ERROR0(ERR_WIA_INVALID,
	    "Hash error for partition header: %s\n",sf->f.fname);

    wia->pte  = calloc(disc->n_part,sizeof(wd_ptab_entry_t));
    wia->part = calloc(disc->n_part,sizeof(wia_part_t));
    if ( !wia->pte || !wia->part )
	OUT_OF_MEMORY;

    int ip;
    const u8 * src = wia->iobuf;
    int shortage = sizeof(wia_part_t) - disc->part_t_size;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wia_part_t *part = wia->part + ip;
	wia_ntoh_part(part,(wia_part_t*)src);
	if ( shortage > 0 )
	    memset( (u8*)part + disc->part_t_size, 0, shortage );
	noTRACE("PT %u.%u, %s\n",
		part->ptab_index, part->ptab_part_index,
		wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO) );

	wd_ptab_entry_t * pte = wia->pte + ip;
	pte->off4  = htonl(part->part_off >> 2);
	pte->ptype = htonl(part->part_type);

	src += disc->part_t_size;
    }


    //----- read and check partition info

    if (disc->part_info_size)
    {
	wia->part_info_size = disc->part_info_size;
	wia->part_info = malloc(disc->part_info_size);
	if (!wia->part_info)
	    OUT_OF_MEMORY;

	ReadAtF(&sf->f,disc->part_info_off,wia->part_info,disc->part_info_size);
	if (err)
	    return err;
	
	SHA1(wia->part_info,disc->part_info_size,hash);
	if (memcmp(hash,disc->part_info_hash,sizeof(hash)))
	    return ERROR0(ERR_WIA_INVALID,
		"Hash error for partition info: %s\n",sf->f.fname);
    }


    //----- finish setup

    wia->is_valid = true;
    SetupMemMap(wia);
    SetupIOD(sf,OFT_WIA,OFT_WIA);

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WriteWIA() + write helpers	///////////////
///////////////////////////////////////////////////////////////////////////////

static wia_data_segment_t * calc_segments
(
    wia_data_segment_t	* seg,		// destination segment pointer
    void		* seg_end,	// end of segment space
    const void		* src_ptr,	// pointer to source
    u32			src_size	// size of source
)
{
    DASSERT( seg );
    DASSERT( seg_end );
    DASSERT( src_ptr );
    DASSERT( src_size >= 8 );
    DASSERT( !(src_size&3) );

    const u32 * src = (u32*)src_ptr;
    const u32 * src_end  = src + src_size/4;
    while ( src_end > src && !src_end[-1] )
	src_end--;
    const u32 * src_end2 = src + 2 >= src_end ? src : src_end - 2;
    noPRINT("SRC: %p, end=%x,%x\n", src, src_end2-src, src_end-src );

    while ( src < src_end )
    {
	// skip null data
	while ( src < src_end && !*src )
	    src++;
	if ( src == src_end )
	    break;
	const u32 * src_beg = src;

	// find next hole
	while ( src < src_end2 )
	{
	    if (src[2])
		src += 3;
	    else if (src[1])
		src += 2;
	    else if (src[0])
		src++;
	    else
		break;
	}
	if ( src >= src_end2 )
	    src = src_end;

	const u32 offset	= (u8*)src_beg - (u8*)src_ptr;
	seg->offset		= htonl(offset);
	const u32 size		= (u8*)src - (u8*)src_beg;
	seg->size		= htonl(size);
	noPRINT("SEG: %p: %x+%x\n",seg,offset,size);

	DASSERT(!(size&3));
	DASSERT( seg->data + size < (u8*)seg_end );
	memcpy(seg->data,src_beg,size);
	seg = (wia_data_segment_t*)( seg->data + size );
    }

    //----- last terminating segment

    seg->offset = 0;
    seg->size = 0;
    seg++;
    if ( (u8*)seg > (u8*)seg_end )
	ERROR0(ERR_INTERNAL,"buffer overflow");

    return seg;
}

///////////////////////////////////////////////////////////////////////////////

static enumError write_compressed_data
(
    struct SuperFile_t	* sf,		// destination file
    const void		* data_ptr,	// valid pointer to data
    u32			data_size,	// size of data to write
    u32			* write_count	// not NULL: store written data count
)
{
    DASSERT( sf );
    DASSERT( data_ptr );
    DASSERT( !(data_size&3) );

    wia_controller_t * wia = sf->wia;
    DASSERT(wia);

    switch(wia->disc.compression)
    {
	case WIA_COMPR_BZIP2:
	{
	    ASSERT(sf->f.fp);
	    enumError err = SeekF(&sf->f,wia->write_data_off);
	    if (err)
		return err;

	    int bzerror;
	    BZFILE *bz = BZ2_bzWriteOpen(&bzerror,sf->f.fp,9,0,0);
	    if ( !bz || bzerror != BZ_OK )
	    {
		if (bz)
		    BZ2_bzWriteClose(0,bz,0,0,0);
		return ERROR0(ERR_BZIP2,
			"Error while opening bzip2 stream: %s\n",sf->f.fname);
	    }

	    BZ2_bzWrite(&bzerror,bz,(u8*)data_ptr,data_size);
	    if ( bzerror != BZ_OK )
		return ERROR0(ERR_BZIP2,
			"Error while writing bzip2 stream: %s\n",sf->f.fname);

	    unsigned int nbytes_out;
	    BZ2_bzWriteClose(&bzerror,bz,0,0,&nbytes_out);
	    if ( bzerror != BZ_OK )
		return ERROR0(ERR_BZIP2,
			"Error while closing bzip2 stream: %s\n",sf->f.fname);

	    sf->f.max_off = wia->write_data_off + nbytes_out;
	    wia->write_data_off += nbytes_out + 3 & ~3;
	    if (write_count)
		*write_count = nbytes_out;
	}
	break;

	default:
	{

	    enumError err = WriteAtF( &sf->f, wia->write_data_off, data_ptr, data_size );
	    if (err)
		return err;

	    sha1_hash hash;
	    SHA1(data_ptr,data_size,hash);
	    //HEXDUMP16(0,0,data_ptr,16);
	    //HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);

	    err = WriteF( &sf->f, hash, WII_HASH_SIZE );
	    if (err)
		return err;

	    data_size += WII_HASH_SIZE;
	    wia->write_data_off += data_size;
	    if (write_count)
		*write_count = data_size;
	}
	break;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError write_part_data
(
    struct SuperFile_t	* sf		// destination file
)
{
    DASSERT(sf);
    DASSERT(sf->wia);
    wia_controller_t * wia = sf->wia;
    enumError err = ERR_OK;

    if ( wia->gdata_group != ~(u32)0 && wia->gdata_part_index < wia->disc.n_part )
    {
	//----- decrpyt and split data

	wd_disc_t * wdisc = wia->wdisc;
	ASSERT(wdisc);
	ASSERT( wia->gdata_part_index < wdisc->n_part );
	wd_part_t * wpart = wdisc->part + wia->gdata_part_index;

	u8 hashtab1[WII_GROUP_HASH_SIZE];
	if ( wpart->is_encrypted )
	    wd_decrypt_sectors(wpart,0,wia->gdata,wia->gdata,hashtab1,WII_GROUP_SECTORS);
	else
	    wd_split_sectors(wia->gdata,wia->gdata,hashtab1,WII_GROUP_SECTORS);


	//----- setup exceptions
	
	wia_data_t * data = (wia_data_t*)wia->iobuf;
	wia_exception_t * except = data->exception;

	u8 hashtab2[WII_GROUP_HASH_SIZE];
	memset(hashtab2,0xdc,0x100);
	wd_calc_group_hashes(wia->gdata,hashtab2,0,0);


	int is;
	for ( is = 0; is < WII_GROUP_SECTORS; is++ )
	{
	    wd_part_sector_t * sector1
		= (wd_part_sector_t*)( hashtab1 + is * WII_SECTOR_HASH_SIZE );
	    wd_part_sector_t * sector2
		= (wd_part_sector_t*)( hashtab2 + is * WII_SECTOR_HASH_SIZE );

	    int ih;
	    u8 * h1 = sector1->h0[0];
	    u8 * h2 = sector2->h0[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H0; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H0.%02u -> %04x,%04x\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }

	    h1 = sector1->h1[0];
	    h2 = sector2->h1[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H1; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H1.%u  -> %04x,%04x\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }

	    h1 = sector1->h2[0];
	    h2 = sector2->h2[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H2; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H2.%u  -> %04x,%04x\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }
	}
	data->n_exceptions = htons( except - data->exception );


	//----- align segment start

	u32 data_size = (u8*)except - wia->iobuf + 3 & ~3;
	data->seg_offset = htonl(data_size);
	wia_data_segment_t * seg = (wia_data_segment_t*)( wia->iobuf + data_size );


	//----- calc data segments

	seg = calc_segments( seg, wia->iobuf + sizeof(wia->iobuf),
				wia->gdata, WII_GROUP_DATA_SIZE );


	//----- compression

	data_size = (u8*)seg - wia->iobuf;
	ASSERT( data_size <= sizeof(wia->iobuf) );
	DASSERT(!(data_size&3));

	const u64 write_data_off = wia->write_data_off;
	err = write_compressed_data( sf, wia->iobuf, data_size, &data_size );
	if (err)
	    return err;


	//----- set group info

	wia_part_t  * part = wia->part + wia->gdata_part_index;
	wia_group_t * grp  = ( wia_group_t *) ( wia->part_info + part->group_off )
			   +  wia->gdata_group;
	ASSERT( (u8*)grp + sizeof(*grp) <= wia->part_info + wia->part_info_size );
	grp->data_off4  = htonl( write_data_off >> 2 );
	grp->data_size  = htonl(data_size);

	PRINT(" GRP=%5u[%zx], N(exceptions)=%4u, DECRYPT=%d, off=%llx, datasize=%x=%u\n",
		wia->gdata_group, (u8*)grp - wia->part_info,
		ntohs(data->n_exceptions), wpart->is_encrypted,
		wia->write_data_off - data_size, data_size, data_size );
    }

    wia->gdata_part_index	= 0;
    wia->gdata_group		= ~(u32)0;
    memset(wia->gdata,0,sizeof(wia->gdata));

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteWIA()",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx\n",(u64)sf->f.file_off,(u64)sf->f.max_off);

    wia_controller_t * wia = sf->wia;
    const u64 off2 = off + count;
    const wd_patch_item_t * item = wia->memmap.item;
    const wd_patch_item_t * item_end = item + wia->memmap.used;

    for ( ; item < item_end && item->offset < off2; item++ )
    {
	const u64 end = item->offset + item->size;
	noTRACE("> off=%llx..%llx, item=%llx..%llx\n", (u64)off, off2, item->offset, end );
	if ( item->offset < off2 && end > off )
	{
	    u64 overlap1 = item->offset > off ? item->offset : off;
	    const u64 overlap2 = end < off2 ? end : off2;
	    noTRACE(" -> %llx .. %llx\n",overlap1,overlap2);
	    sf->f.bytes_written += overlap2 - overlap1;

	    switch (item->mode)
	    {
	      case WD_PAT_DATA:
		PRINT(" > WRITE DISC: %llx .. %llx\n",overlap1,overlap2);
		memcpy(	(u8*)item->data + (overlap1-item->offset),
			(ccp)buf + (overlap1-off),
			overlap2 - overlap1 );
		break;

	      case WD_PAT_PART_DATA:
		while ( overlap1 < overlap2 )
		{
		    ASSERT( item->part_index < wia->disc.n_part );
		    ASSERT( wia->part );
		    wia_part_t * part = wia->part + item->part_index;
		    u32 sector = overlap1 / WII_SECTOR_SIZE - part->first_sector;
		    const u32 group = sector / WII_GROUP_SECTORS;
		    if ( item->part_index != wia->gdata_part_index || group != wia->gdata_group )
		    {
			const enumError err = write_part_data(sf);
			if (err)
			    return err;
			wia->gdata_part_index = item->part_index;
			wia->gdata_group = group;
		    }
		    
		    sector -= group * WII_GROUP_SECTORS;
		    const u64 group_off1 = group * (u64)WII_GROUP_SIZE
					 + part->first_sector * (u64)WII_SECTOR_SIZE;
		    u64 group_off2 = group_off1 + WII_GROUP_SIZE;
		    if ( group_off2 > overlap2 )
			group_off2 = overlap2;

		    TRACE(" > COLLECT P%u/%u.%u: %llx .. %llx -> %llx + %llx\n",
				item->part_index, group, sector,
				overlap1, group_off2,
				overlap1 - group_off1, group_off2 - overlap1 );
		    memcpy(	wia->gdata + (overlap1-group_off1),
				(ccp)buf + (overlap1-off),
				group_off2 - overlap1 );
		    overlap1 = group_off2;
		}
		break;

	      default:
		ASSERT(0);
	    }
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
)
{
    return off + count < WII_PART_OFF
		? WriteWIA(sf,off,buf,count)
		: SparseHelper(sf,off,buf,count,WriteWIA,WIA_MIN_HOLE_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroWIA 
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteZeroWIA()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx,%llx\n",
		(u64)sf->f.file_off, (u64)sf->f.max_off,
		(u64)sf->max_virt_off);

    // [2do] [wia]
    return ERROR0(ERR_NOT_IMPLEMENTED,"WIA is not supported yet.\n");
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup and term write WIA	///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteWIA
(
    struct SuperFile_t	* sf,		// file to setup
    struct SuperFile_t	* src		// NULL or source file
)
{
    ASSERT(sf);
    PRINT("#W# SetupWriteWIA(%p,%p) oft=%x, wia=%p, v=%s/%s\n",
		sf, src, sf->iod.oft, sf->wia,
		PrintVersionWIA(0,0,WIA_VERSION_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION) );

    if (sf->wia)
	return ERROR0(ERR_INTERNAL,0);

    if (!src)
	return ERROR0(ERR_INTERNAL,"Missing source info\n");

    wd_disc_t * wdisc = OpenDiscSF(src,true,true);
    if (!wdisc)
	return ERR_WDISC_NOT_FOUND;

    if (wdisc->have_overlays)
	return ERROR0(ERR_NO_WIA_SUPPORT,
			"No WIA support for overlayed partitions: %s\n",
			src->f.fname );

    CleanSF(sf);
    OpenStreamFile(&sf->f);


    //----- setup controller

    wia_controller_t * wia = calloc(1,sizeof(*wia));
    if (!wia)
	return OUT_OF_MEMORY;
    sf->wia = wia;
    wia->wdisc = wdisc;
    wia->gdata_group = ~(u32)0;


    //----- setup file header

    wia_file_head_t *fhead = &wia->fhead;
    memcpy(fhead->magic,WIA_MAGIC,sizeof(fhead->magic));
    fhead->magic[3]++; // magic is invalid now
    fhead->version		= WIA_VERSION;
    fhead->version_compatible	= WIA_VERSION_COMPATIBLE;
    fhead->iso_size		= src->file_size;


    //----- setup disc info

    wia_disc_t *disc = &wia->disc;
    disc->compression	= opt_no_compress ? WIA_COMPR_NONE : WIA_COMPR__DEFAULT;
    disc->n_part	= wdisc->n_part;

    memcpy(&disc->dhead,&wdisc->dhead,sizeof(disc->dhead));


    //----- setup part info

    if (!disc->n_part)
	return ERROR0(ERR_NO_WIA_SUPPORT,
		"No WIA support for discs without partitions: %s\n",
		src->f.fname );
    
    int pi;
    for ( pi = 0; pi < disc->n_part; pi++ )
	if (!wdisc->part[pi].is_valid)
	    return ERROR0(ERR_NO_WIA_SUPPORT,
			"No WIA support for discs with invalid partitions: %s\n",
			src->f.fname );

    wd_ptab_entry_t * pte  = calloc(disc->n_part,sizeof(wd_ptab_entry_t));
    wia_part_t	    * part = calloc(disc->n_part,sizeof(wia_part_t));
    if ( !pte || !part )
	OUT_OF_MEMORY;
    wia->pte  = pte;
    wia->part = part;

    u32 part_info_size = 0;
    for ( pi = 0; pi < disc->n_part; pi++, part++, pte++ )
    {
	wd_part_t * wpart	= wdisc->part + pi;
	wd_load_part(wpart,true,true);

	pte->off4		= htonl(wpart->part_off4);
	pte->ptype		= htonl(wpart->part_type);

	part->ptab_index	= wpart->ptab_index;
	part->ptab_part_index	= wpart->ptab_part_index;
	part->part_type		= wpart->part_type;
	part->part_off		= (u64)wpart->part_off4 << 2;
	memcpy(part->part_key,wpart->key,sizeof(part->part_key));

	part->first_sector	= wpart->data_sector;
	part->n_sectors		= wpart->end_sector - wpart->data_sector;
	part->n_groups		= ( part->n_sectors + WII_GROUP_SECTORS - 1 )
				/ WII_GROUP_SECTORS;
	
	noTRACE("PT %u.%u, %s / sect = %x,%x,%x\n",
		part->ptab_index, part->ptab_part_index,
		wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
		part->first_sector, part->n_sectors, part->n_groups );

	//----- assign part_info parameters, align all at multiple of 4

	part->ticket_off = part_info_size;
	part_info_size	+= sizeof(wd_part_header_t) + 3 & ~(u32)3;

	if (wpart->ph.tmd_off4)
	{
	    part->tmd_off   = part_info_size;
	    part->tmd_size  = wpart->ph.tmd_size;
	    part_info_size += wpart->ph.tmd_size + 3 & ~(u32)3;
	}

	if (wpart->ph.cert_off4)
	{
	    part->cert_off   = part_info_size;
	    part->cert_size  = wpart->ph.cert_size;
	    part_info_size  += wpart->ph.cert_size + 3 & ~(u32)3;
	}

	if (wpart->ph.h3_off4)
	{
	    part->h3_off    = part_info_size;
	    part_info_size += WII_H3_SIZE + 3 & ~(u32)3;
	}
	
	part->group_off = part_info_size;
	part_info_size += part->n_groups * sizeof(wia_group_t);
    }

    u8 * part_info = malloc(part_info_size);
    if (!part_info)
	OUT_OF_MEMORY;
    memset(part_info,0,part_info_size);
    wia->part_info = part_info;
    wia->part_info_size = part_info_size;

    for ( pi = 0, part = wia->part; pi < disc->n_part; pi++, part++ )
    {
	DASSERT( pi < disc->n_part - 1
		|| part->group_off + part->n_groups * sizeof(wia_group_t) );
	ASSERT( part->group_off + part->n_groups * sizeof(wia_group_t) <= part_info_size );

	wd_part_t * wpart = wdisc->part + pi;
	noTRACE("P#%u: %x,%x,%x,%x\n",
		pi, part->ticket_off, part->tmd_off, part->cert_off, part->h3_off );

	wd_part_header_t * ph = (wd_part_header_t*)( part_info + part->ticket_off );
	memcpy( ph, &wpart->ph, sizeof(*ph) );
	hton_part_header(ph,ph);

	if (wpart->ph.tmd_off4)
	    memcpy( part_info + part->tmd_off, wpart->tmd, wpart->ph.tmd_size );

	if (wpart->ph.cert_off4)
	    memcpy( part_info + part->cert_off, wpart->cert, wpart->ph.cert_size );

	if (wpart->ph.h3_off4)
	    memcpy( part_info + part->h3_off, wpart->h3, WII_H3_SIZE );
    }


    //----- finish setup

    wia->write_data_off	= sizeof(wia_file_head_t)
			+ sizeof(wia_disc_t)
			+ sizeof(wia_part_t) * disc->n_part
			+ part_info_size;

    wia->is_valid = true;
    SetupMemMap(wia);
    SetupIOD(sf,OFT_WIA,OFT_WIA);

    if ( verbose >= 0 )
	ERROR0(ERR_WARNING,
		"*******************************************\n"
		"***  The WIA support is EXPERIMENTAL!   ***\n"
		"***  The WIA format is in development!  ***\n"
		"***  Don't use WIA files productive!    ***\n"
		"*******************************************\n" );

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError TermWriteWIA 
(
    struct SuperFile_t	* sf		// file to terminate
)
{
    ASSERT(sf);
    ASSERT(sf->wia);
    TRACE("#W# TermWriteWIA(%p)\n",sf);

    wia_controller_t * wia = sf->wia;
    wia_disc_t *disc = &wia->disc;


    //----- write chached partition data

    enumError err = write_part_data(sf);
    if (err)
	return err;


    //----- write disc data

    wia_data_segment_t * seg = (wia_data_segment_t*) wia->iobuf;
    seg = calc_segments( seg, wia->iobuf + sizeof(wia->iobuf),
			wia->disc_data, sizeof(wia->disc_data) );

    //PRINT("DISC DATA:"); HEXDUMP16(0,0x80,wia->iobuf,16);
    disc->disc_data_off = wia->write_data_off;
    err = write_compressed_data( sf, wia->iobuf, (u8*)seg - wia->iobuf, &disc->disc_data_size );
    if (err)
	return err;

    PRINT("DISC DATA: %llx + %x\n", disc->disc_data_off, disc->disc_data_size );
    

    //----- calc part header

    int pi;
    for ( pi = 0; pi < disc->n_part; pi++ )
    {
	wia_part_t * part = wia->part + pi;
	wia_hton_part(part,part);
    }

    const u32 total_part_size = sizeof(wia_part_t) * disc->n_part;
    const u64 part_off = sizeof(wia_file_head_t) + sizeof(wia_disc_t);


    //----- calc disc info

    disc->part_t_size		= sizeof(wia_part_t);
    disc->part_off		= part_off;
    SHA1((u8*)wia->part,total_part_size,disc->part_hash);

    const u32 part_info_off	= disc->part_off + total_part_size;
    disc->part_info_off		= part_info_off;
    disc->part_info_size	= wia->part_info_size;
    SHA1(wia->part_info,wia->part_info_size,disc->part_info_hash);

    wia_hton_disc(disc,disc);


    //----- calc file header

    wia_file_head_t *fhead	= &wia->fhead;
    memcpy(fhead->magic,WIA_MAGIC,sizeof(fhead->magic));
    fhead->version		= WIA_VERSION;
    fhead->version_compatible	= WIA_VERSION_COMPATIBLE;
    fhead->disc_size		= sizeof(wia_disc_t);
    fhead->file_size		= sf->f.max_off;

    SHA1((u8*)disc,sizeof(*disc),fhead->disc_hash);
    wia_hton_file_head(fhead,fhead);

    SHA1((u8*)fhead, sizeof(*fhead)-sizeof(fhead->file_head_hash),
		fhead->file_head_hash );


    //----- write data

    err = WriteAtF(&sf->f,0,fhead,sizeof(*fhead));
    if (err)
	return err;

    WriteAtF(&sf->f,sizeof(*fhead),disc,sizeof(*disc));
    if (err)
	return err;

    WriteAtF(&sf->f,part_off,wia->part,total_part_size);
    if (err)
	return err;

    WriteAtF(&sf->f,part_info_off,wia->part_info,wia->part_info_size);
    if (err)
	return err;


    // convert back to local endian

    wia_ntoh_file_head(fhead,fhead);
    wia_ntoh_disc(disc,disc);
    for ( pi = 0; pi < disc->n_part; pi++ )
    {
	wia_part_t * part = wia->part + pi;
	wia_ntoh_part(part,part);
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_file_head ( wia_file_head_t * dest, const wia_file_head_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->version		= ntohl (src->version);
    dest->version_compatible	= ntohl (src->version_compatible);
    dest->disc_size		= ntohl (src->disc_size);
    dest->file_size		= ntoh64(src->file_size);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_file_head ( wia_file_head_t * dest, const wia_file_head_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->version		= htonl (src->version);
    dest->version_compatible	= htonl (src->version_compatible);
    dest->disc_size		= htonl (src->disc_size);
    dest->file_size		= hton64(src->file_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_disc ( wia_disc_t * dest, const wia_disc_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->compression		= ntohl (src->compression);

    dest->n_raw_data		= ntohl (src->n_raw_data);
    dest->raw_data_off		= ntoh64(src->raw_data_off);

    dest->n_part		= ntohl (src->n_part);
    dest->part_t_size		= ntohl (src->part_t_size);
    dest->part_off		= ntoh64(src->part_off);
    dest->part_info_off		= ntoh64(src->part_info_off);
    dest->part_info_size	= ntohl (src->part_info_size);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_disc ( wia_disc_t * dest, const wia_disc_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->compression		= htonl (src->compression);

    dest->n_raw_data		= htonl (src->n_raw_data);
    dest->raw_data_off		= hton64(src->raw_data_off);

    dest->n_part		= htonl (src->n_part);
    dest->part_t_size		= htonl (src->part_t_size);
    dest->part_off		= hton64(src->part_off);
    dest->part_info_off		= hton64(src->part_info_off);
    dest->part_info_size	= htonl (src->part_info_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_part ( wia_part_t * dest, const wia_part_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->ptab_index		= ntohl (src->ptab_index);
    dest->ptab_part_index	= ntohl (src->ptab_part_index);
    dest->part_type		= ntohl (src->part_type);
    dest->part_off		= ntoh64(src->part_off);

    dest->first_sector		= ntohl (src->first_sector);
    dest->n_sectors		= ntohl (src->n_sectors);
    dest->n_groups		= ntohl (src->n_groups);

    dest->ticket_off		= ntohl (src->ticket_off);
    dest->tmd_off		= ntohl (src->tmd_off);
    dest->tmd_size		= ntohl (src->tmd_size);
    dest->cert_off		= ntohl (src->cert_off);
    dest->cert_size		= ntohl (src->cert_size);
    dest->h3_off		= ntohl (src->h3_off);
    dest->group_off		= ntohl (src->group_off);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_part ( wia_part_t * dest, const wia_part_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->ptab_index		= htonl (src->ptab_index);
    dest->ptab_part_index	= htonl (src->ptab_part_index);
    dest->part_type		= htonl (src->part_type);
    dest->part_off		= hton64(src->part_off);

    dest->first_sector		= htonl (src->first_sector);
    dest->n_sectors		= htonl (src->n_sectors);
    dest->n_groups		= htonl (src->n_groups);

    dest->ticket_off		= htonl (src->ticket_off);
    dest->tmd_off		= htonl (src->tmd_off);
    dest->tmd_size		= htonl (src->tmd_size);
    dest->cert_off		= htonl (src->cert_off);
    dest->cert_size		= htonl (src->cert_size);
    dest->h3_off		= htonl (src->h3_off);
    dest->group_off		= htonl (src->group_off);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

