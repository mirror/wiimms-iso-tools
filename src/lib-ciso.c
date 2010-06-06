
#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "debug.h"
#include "libwbfs.h"
#include "lib-sf.h"
#include "wbfs-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    CISO support                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError InitializeCISO ( CISO_Info_t * ci, CISO_Head_t * ch )
{
    ASSERT(ci);

    memset(ci,0,sizeof(*ci));
    return SetupCISO(ci,ch);
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupCISO ( CISO_Info_t * ci, CISO_Head_t * ch )
{
    ASSERT(ci);
    TRACE("SetupCISO(%p,%p)\n",ci,ch);

    ResetCISO(ci);
    if (!ch)
	return ERR_OK;
    
    if (memcmp(&ch->magic,"CISO",4))
    {
	TRACE(" - NO CISO\n");
	return ERR_NO_CISO;
    }

    // block size is stored in untypical little endian
    u8 * bs = (u8*)&ch->block_size;
    const u32 block_size = ((( bs[3] << 8 ) | bs[2] ) << 8 | bs[1] ) << 8 | bs[0];

    ASSERT( !( CISO_MIN_BLOCK_SIZE & CISO_MIN_BLOCK_SIZE-1 ) );
    if ( block_size < CISO_MIN_BLOCK_SIZE || block_size & CISO_MIN_BLOCK_SIZE-1 )
    {
	TRACE(" - INVALIDS CISO, block_size=%x\n",block_size);
	return ERR_CISO_INVALID;
    }

    u32 used_blocks = 0;
    const u8 *bptr, *bend = ch->map + CISO_MAP_SIZE, *bmax = ch->map;
    for( bptr = ch->map; bptr < bend; bptr++ )
	if ( *bptr == 1 )
	{
	    bmax = bptr + 1;
	    used_blocks++;
	}
	else if (*bptr)
	{
	    TRACE(" - INVALID CISO, map[%04zx] = %02x\n", bptr-ch->map, *bptr );
	    ResetCISO(ci);
	    return ERR_CISO_INVALID;
	}

    ci->block_size	= block_size;
    ci->used_blocks	= used_blocks;
    ci->needed_blocks	= bmax - ch->map;
    ci->max_file_off	= (u64)block_size * used_blocks + CISO_HEAD_SIZE;
    ci->max_virt_off	= (u64)block_size * ci->needed_blocks;
    
    if (ci->needed_blocks)
    {
	ci->map_size = ci->needed_blocks;
	ci->map = malloc( ci->map_size * sizeof(*ci->map) );
	if (!ci->map)
	    OUT_OF_MEMORY;

	CISO_Map_t * mptr = ci->map; u16 mcount = 0;
	bend = ch->map + ci->map_size;
	for( bptr = ch->map; bptr < bend; bptr++, mptr++ )
	    *mptr = *bptr ? mcount++ : CISO_UNUSED_BLOCK;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void ResetCISO ( CISO_Info_t * ci )
{
    ASSERT(ci);

    if (ci->map)
	free(ci->map);
    memset(ci,0,sizeof(*ci));
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      read CISO                  ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadCISO ( SuperFile_t * sf )
{
    ASSERT(sf);
    ASSERT( sizeof(CISO_Head_t) == CISO_HEAD_SIZE );

    CISO_Info_t *ci = &sf->ciso;
    TRACE("#C# SetupReadCISO(%p) ci.map=%p\n",sf,ci->map);
    if ( ci->map )
	return ERR_OK;

    FreeSF(sf);

    CISO_Head_t ch;
    enumError err = ReadAtF(&sf->f,0,&ch,sizeof(ch));
    if (err)
	return err;
    TRACE("#C#  - header read\n");

    if (SetupCISO(&sf->ciso,&ch))
	return ERROR0(ERR_CISO_INVALID,"Invalid CISO file\n");

    const off_t min_size = (off_t)WII_SECTORS_SINGLE_LAYER * WII_SECTOR_SIZE;

    sf->file_size	= ci->max_file_off > min_size ? ci->max_file_off : min_size;
    sf->f.max_off	= ci->max_file_off;
    sf->max_virt_off	= ci->max_virt_off;
    SetupIOD(sf,OFT_CISO,OFT_CISO);
    SetupISOModifier(sf);

    TRACE("#C# CISO FOUND!\n");
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadCISO ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    CISO_Info_t *ci = &sf->ciso;
    if (!ci->map)
	return ReadAtF(&sf->f,off,buf,count);

    TRACE("#C# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#C# ReadCISO()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(ci->block_size);
    const u64 block_size = ci->block_size;

    char * dest = buf;
    u32 block = off/block_size;
    while ( count > 0 )
    {
	if ( block >= ci->map_size )
	{
	    TRACE("#C# >FILL %p +%8zx = %p .. %zx, bl=%x/%x\n",
		buf, dest-(ccp)buf, dest, count, block, ci->map_size );
	    memset(dest,0,count);
	    return ERR_OK;
	}
	
	const size_t beg = off - block * block_size;
	ASSERT( beg < block_size );
	size_t size = block_size - beg;
	ASSERT(size>0);
	if ( size > count )
	    size = count;

	CISO_Map_t rd_block = ci->map[block];

	TRACE("#C# >EXPECT bl=%x/%x -> %x / off=%llx beg=%zx, size=%zx\n",
			block, ci->map_size, rd_block,
			(u64)off, beg, size );

	if ( rd_block == CISO_UNUSED_BLOCK )
	{
	    TRACE("#C# >FILL %p +%8zx = %p .. %zx\n",
			buf, dest-(ccp)buf, dest, size );
	    memset(dest,0,size);
	}
	else
	{
	    TRACE("#C# >READ %p +%8zx = %p .. %zx\n",
			buf, dest-(ccp)buf, dest, size );
	    const enumError err
		= ReadAtF(&sf->f,rd_block*block_size+CISO_HEAD_SIZE+beg,dest,size);
	    if (err)
		return err;
	}

	off   += size;
	dest  += size;
	count -= size;
	block++;
    }

    TRACE("#C# DONE\n");
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     write CISO                  ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteCISO ( SuperFile_t * sf )
{
    ASSERT(sf);
    ASSERT( sizeof(CISO_Head_t) == CISO_HEAD_SIZE );

    CISO_Info_t *ci = &sf->ciso;
    TRACE("#C# SetupWriteCISO(%p) ci.map=%p\n",sf,ci->map);

    if ( ci->map_size == CISO_MAP_SIZE && !ci->needed_blocks )
	return ERR_OK; // setup already done

    ResetCISO(ci);
    ASSERT(!ci->map);

    //---- setup map

    ci->map_size = CISO_MAP_SIZE;
    ci->map = malloc( ci->map_size * sizeof(*ci->map) );
    if (!ci->map)
	OUT_OF_MEMORY;

    int i;
    for ( i = 0; i < ci->map_size; i++ )
	ci->map[i] = CISO_UNUSED_BLOCK;

    //---- calc block size

    const u64 max_file_size = WII_MAX_SECTORS * (u64)WII_SECTOR_SIZE;
    u32 block_size = max_file_size / CISO_MAP_SIZE;
    TRACE(" - %8x minimal block_size\n",block_size);

    const u64 file_size = sf->file_size ? sf->file_size : max_file_size;
    TRACE(" -          file size = %llx\n",file_size);
    u32 temp = file_size / CISO_WR_MAX_BLOCK;
    if ( block_size < temp )
    {
	block_size = temp;
	TRACE(" - %8x new block_size\n",block_size);
    }

    block_size = ( (block_size-1) / CISO_MIN_BLOCK_SIZE + 1 ) * CISO_MIN_BLOCK_SIZE;
    TRACE(" - %8x aligned block_size\n",block_size);

 #if 0 // [2do] [obsolete?]

    if ( block_size < CISO_WR_MIN_BLOCK_SIZE )
    {   
	block_size = CISO_WR_MIN_BLOCK_SIZE;
	TRACE(" - %8x CISO_WR_MIN_BLOCK_SIZE\n",block_size);
    }

    ci->block_size = block_size;

 #else

    // and now force power of 2

    ci->block_size = CISO_WR_MIN_BLOCK_SIZE;
    while ( ci->block_size < block_size )
	ci->block_size <<= 1;
    TRACE(" - %8x power2 block_size\n",ci->block_size);

 #endif

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError TermWriteCISO ( SuperFile_t * sf )
{
    ASSERT(sf);
    CISO_Info_t *ci = &sf->ciso;
    ASSERT(ci->map);
    TRACE("#C# TermWriteCISO(%p)\n",sf);

    CISO_Head_t ch;
    memset(&ch,0,sizeof(ch));
    memcpy(&ch.magic,"CISO",4);

    // block size is stored in untypical little endian
    u32 temp = ci->block_size;
    u8 * dest = (u8*)&ch.block_size;
    *dest++ = temp & 0xff; temp >>= 8;
    *dest++ = temp & 0xff; temp >>= 8;
    *dest++ = temp & 0xff; temp >>= 8;
    *dest++ = temp & 0xff;

    const CISO_Map_t * src = ci->map;
    const CISO_Map_t * end = src + ci->needed_blocks;
    while ( src < end )
	*dest++ = *src++ != CISO_UNUSED_BLOCK;
    ASSERT( dest - (u8*)&ch <= sizeof(ch) );
    ASSERT( dest == ch.map + ci->needed_blocks );

    const enumError err = WriteAtF(&sf->f,0,&ch,sizeof(ch));
    if (!err)
	SetSizeF(&sf->f, (u64)ci->block_size * ci->used_blocks + CISO_HEAD_SIZE );
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteCISO ( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    CISO_Info_t *ci = &sf->ciso;
    if (!ci->map)
	return WriteAtF(&sf->f,off,buf,count);

    TRACE("#C# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#C# WriteCISO()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx\n",(u64)sf->f.file_off,(u64)sf->f.max_off);

    if (!count)
	return ERR_OK;

    // adjust the file size
    const off_t data_end = off + count;
    if ( sf->file_size < data_end )
	sf->file_size = data_end;

    ASSERT(ci->block_size);
    const u64 block_size = ci->block_size;

    ccp src = buf;
    u32 block = off/block_size;
    while ( count > 0 )
    {
	if ( block >= ci->map_size )
	{
	    if ( !sf->f.disable_errors && sf->f.last_error != ERR_WRITE_FAILED )
		ERROR1 ( ERR_WRITE_FAILED,
			"File to large [%c=%d, block=%u/%u]: %s\n",
			GetFT(&sf->f), GetFD(&sf->f),
			sf->f.fname, block, ci->map_size );
	    return sf->f.last_error = ERR_WRITE_FAILED;
	}

	const size_t beg = off - block * block_size;
	ASSERT( beg < block_size );
	size_t size = block_size - beg;
	ASSERT(size>0);
	if ( size > count )
	    size = count;

	if ( block >= ci->needed_blocks )
	{
	    // append a new block

	    ci->needed_blocks = block + 1;
	    const u32 dest_block = ci->used_blocks++;
	    ci->map[block] = dest_block;
	    u64 dest_off = dest_block * block_size + CISO_HEAD_SIZE + beg;
	    TRACE(" ==> APPEND BLOCK: map[%u] = %u ==> off=%llx\n",
			block, dest_block, dest_off);
	    const enumError err = WriteAtF(&sf->f,dest_off,src,size);
	    if (err)
		return err;
	}
	else if ( ci->map[block] != CISO_UNUSED_BLOCK )
	{
	    // modify an already written block

	    u64 dest_off = ci->map[block] * block_size + CISO_HEAD_SIZE + beg;
	    TRACE(" ==> MODIFY BLOCK: map[%u] = %u ==> off=%llx\n",
			block, ci->map[block], dest_off);
	    const enumError err = WriteAtF(&sf->f,dest_off,src,size);
	    if (err)
		return err;
	}
	else
	{
	    // try to modify a skipped block ==> ERR_WRITE_FAILED

	    if ( !sf->f.disable_errors )
		ERROR1( ERR_WRITE_FAILED,
			"Can't write to already skiped block [%c=%d, block=%u/%u]: %s\n",
			GetFT(&sf->f), GetFD(&sf->f),
			sf->f.fname, block, ci->map_size );
	    return sf->f.last_error = ERR_WRITE_FAILED;
	}

	off   += size;
	src   += size;
	count -= size;
	block++;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseCISO
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    return SparseHelper(sf,off,buf,count,WriteCISO,CISO_WR_MIN_HOLE_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroCISO ( SuperFile_t * sf, off_t off, size_t size )
{
    // [2do] [zero] optimization

    while ( size > 0 )
    {
	const size_t size1 = size < sizeof(zerobuf) ? size : sizeof(zerobuf);
	const enumError err = WriteCISO(sf,off,zerobuf,size1);
	if (err)
	    return err;
	off  += size1;
	size -= size1;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

