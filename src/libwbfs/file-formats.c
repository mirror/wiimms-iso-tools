
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

#include "file-formats.h"
#include "crypt.h"

///////////////////////////////////////////////////////////////////////////////
///////////////				setup			///////////////
///////////////////////////////////////////////////////////////////////////////

int validate_file_format_sizes ( int trace_sizes )
{
    // 1. trace sizeof

 #ifdef TRACE_SIZEOF
    if (trace_sizes)
    {
	TRACE_SIZEOF(u8);
	TRACE_SIZEOF(u16);
	TRACE_SIZEOF(u32);
	TRACE_SIZEOF(u64);

	TRACE_SIZEOF(be16_t);
	TRACE_SIZEOF(be32_t);
	TRACE_SIZEOF(be64_t);

	TRACE_SIZEOF(dol_header_t);
	TRACE_SIZEOF(wbfs_inode_info_t);
	TRACE_SIZEOF(wd_header_t);
	TRACE_SIZEOF(wd_boot_t);
	TRACE_SIZEOF(wd_region_t);
	TRACE_SIZEOF(wd_ptab_info_t);
	TRACE_SIZEOF(wd_ptab_entry_t);
	TRACE_SIZEOF(wd_ptab_t);
	TRACE_SIZEOF(wd_ticket_t);
	TRACE_SIZEOF(wd_part_header_t);
	TRACE_SIZEOF(wd_tmd_content_t);
	TRACE_SIZEOF(wd_tmd_t);
	TRACE_SIZEOF(wd_part_control_t);
	TRACE_SIZEOF(wd_part_sector_t);
	TRACE_SIZEOF(wd_fst_item_t);
	TRACE_SIZEOF(wbfs_head_t);
	TRACE_SIZEOF(wbfs_disc_info_t);

     #ifdef DEBUG
	wd_part_control_t pc, pc_saved;
	ASSERT(!clear_part_control(&pc,
			sizeof(wd_tmd_t)+sizeof(wd_tmd_content_t),
			0xa00, 0x1000000 ));
	memcpy(&pc_saved,&pc,sizeof(pc_saved));
	ASSERT(!setup_part_control(&pc));
	ASSERT(!memcmp(&pc_saved,&pc,sizeof(pc_saved)));
     #endif
    }
 #endif

    // 2. assertions

    wd_tmd_t * tmd = 0;
    wd_ticket_t * tik = 0;

    #undef  CHECK
    #define CHECK ASSERT
    #undef  OFFSET
    #define OFFSET(p,i) ((char*)&p->i-(char*)p)

    CHECK( 1 == sizeof(u8)  );
    CHECK( 2 == sizeof(u16) );
    CHECK( 4 == sizeof(u32) );
    CHECK( 8 == sizeof(u64) );

    CHECK( 2 == sizeof(be16_t) );
    CHECK( 4 == sizeof(be32_t) );
    CHECK( 8 == sizeof(be64_t) );

    CHECK( sizeof(wbfs_inode_info_t) + WBFS_INODE_INFO_OFF == 0x100 );
    CHECK( sizeof(wbfs_inode_info_t)	== WBFS_INODE_INFO_SIZE );

    CHECK( sizeof(dol_header_t)		== DOL_HEADER_SIZE );
    CHECK( sizeof(wd_header_t)		== 0x100 );
    CHECK( sizeof(wd_region_t)		== WII_REGION_SIZE );
    CHECK( sizeof(wd_ptab_t)		== WII_MAX_PTAB_SIZE );
    CHECK( sizeof(wd_boot_t)		== WII_BOOT_SIZE );
    CHECK( sizeof(wd_part_sector_t)	== WII_SECTOR_SIZE );
    CHECK( sizeof(wd_fst_item_t)	== 12 ); // test because of union

    CHECK( OFFSET(tik,title_key)	== WII_TICKET_KEY_OFF );
    CHECK( OFFSET(tik,title_id)		== WII_TICKET_IV_OFF );
    CHECK( OFFSET(tik,issuer)		== WII_TICKET_SIG_OFF );
    CHECK( OFFSET(tik,trucha_pad)	== WII_TICKET_BRUTE_FORCE_OFF );
    CHECK( sizeof(wd_ticket_t)		== WII_TICKET_SIZE );

    CHECK( OFFSET(tmd,issuer)		== WII_TMD_SIG_OFF );
    CHECK( OFFSET(tmd,reserved)		== WII_TMD_BRUTE_FORCE_OFF );
    CHECK( OFFSET(tmd,content[0].hash)	== 0x1f4 );
    CHECK( sizeof(wd_tmd_t)		== 0x1e4 );
    CHECK( sizeof(wd_tmd_content_t)	== 0x24 );


    //----- 3. calculate return value

    #undef  CHECK
    #define CHECK(a) if (!(a)) return __LINE__;

    CHECK( 1 == sizeof(u8)  );
    CHECK( 2 == sizeof(u16) );
    CHECK( 4 == sizeof(u32) );
    CHECK( 8 == sizeof(u64) );

    CHECK( 2 == sizeof(be16_t) );
    CHECK( 4 == sizeof(be32_t) );
    CHECK( 8 == sizeof(be64_t) );

    CHECK( sizeof(wbfs_inode_info_t) + WBFS_INODE_INFO_OFF == 0x100 );
    CHECK( sizeof(wbfs_inode_info_t)	== WBFS_INODE_INFO_SIZE );

    CHECK( sizeof(dol_header_t)		== DOL_HEADER_SIZE );
    CHECK( sizeof(wd_header_t)		== 0x100 );
    CHECK( sizeof(wd_region_t)		== WII_REGION_SIZE );
    CHECK( sizeof(wd_ptab_t)		== WII_MAX_PTAB_SIZE );
    CHECK( sizeof(wd_boot_t)		== WII_BOOT_SIZE );
    CHECK( sizeof(wd_part_sector_t)	== WII_SECTOR_SIZE );
    CHECK( sizeof(wd_fst_item_t)	== 12 ); // test because of union

    CHECK( OFFSET(tik,title_key)	== WII_TICKET_KEY_OFF );
    CHECK( OFFSET(tik,title_id)		== WII_TICKET_IV_OFF );
    CHECK( OFFSET(tik,issuer)		== WII_TICKET_SIG_OFF );
    CHECK( OFFSET(tik,trucha_pad)	== WII_TICKET_BRUTE_FORCE_OFF );
    CHECK( sizeof(wd_ticket_t)		== WII_TICKET_SIZE );

    CHECK( OFFSET(tmd,issuer)		== WII_TMD_SIG_OFF );
    CHECK( OFFSET(tmd,reserved)		== WII_TMD_BRUTE_FORCE_OFF );
    CHECK( OFFSET(tmd,content[0].hash)	== 0x1f4 );
    CHECK( sizeof(wd_tmd_t)		== 0x1e4 );
    CHECK( sizeof(wd_tmd_content_t)	== 0x24 );

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		low level endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

u16 be16 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return d[0] << 8 | d[1];
}

u32 be24 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return ( d[0] << 8 | d[1] ) << 8 | d[2];
}

u32 be32 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (( d[0] << 8 | d[1] ) << 8 | d[2] ) << 8 | d[3];
}

u64 be64 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (u64)be32(d) << 32 | be32(d+4);
}

///////////////////////////////////////////////////////////////////////////////

be64_t hton64 ( u64 data )
{
    be64_t result;
    ((u32*)&result)[0] = htonl( (u32)(data >> 32) );
    ((u32*)&result)[1] = htonl( (u32)data );
    return result;
}

u64 ntoh64 ( be64_t data )
{
    return (u64)ntohl(((u32*)&data)[0]) << 32 | ntohl(((u32*)&data)[1]);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		endian conversions for structs		///////////////
///////////////////////////////////////////////////////////////////////////////

void ntoh_dol_header ( dol_header_t * dest, const dol_header_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    const u32 * src_ptr = src->sect_off;
    u32 * dest_ptr = dest->sect_off;
    u32 * dest_end = (u32*)&dest->padding;

    while ( dest_ptr < dest_end )
	*dest_ptr++ = ntohl(*src_ptr++);
}

//-----------------------------------------------------------------------------

void hton_dol_header ( dol_header_t * dest, const dol_header_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    const u32 * src_ptr = src->sect_off;
    u32 * dest_ptr = dest->sect_off;
    u32 * dest_end = (u32*)&dest->padding;

    while ( dest_ptr < dest_end )
	*dest_ptr++ = htonl(*src_ptr++);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_boot ( wd_boot_t * dest, const wd_boot_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->dol_off4	= ntohl(src->dol_off4);
    dest->fst_off4	= ntohl(src->fst_off4);
    dest->fst_size4	= ntohl(src->fst_size4);
}

//-----------------------------------------------------------------------------

void hton_boot ( wd_boot_t * dest, const wd_boot_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->dol_off4	= htonl(src->dol_off4);
    dest->fst_off4	= htonl(src->fst_off4);
    dest->fst_size4	= htonl(src->fst_size4);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_part_header ( wd_part_header_t * dest, const wd_part_header_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->tmd_size	= ntohl(src->tmd_size);
    dest->tmd_off4	= ntohl(src->tmd_off4);
    dest->cert_size	= ntohl(src->cert_size);
    dest->cert_off4	= ntohl(src->cert_off4);
    dest->h3_off4	= ntohl(src->h3_off4);
    dest->data_off4	= ntohl(src->data_off4);
    dest->data_size4	= ntohl(src->data_size4);
}

//-----------------------------------------------------------------------------

void hton_part_header ( wd_part_header_t * dest, const wd_part_header_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->tmd_size	= htonl(src->tmd_size);
    dest->tmd_off4	= htonl(src->tmd_off4);
    dest->cert_size	= htonl(src->cert_size);
    dest->cert_off4	= htonl(src->cert_off4);
    dest->h3_off4	= htonl(src->h3_off4);
    dest->data_off4	= htonl(src->data_off4);
    dest->data_size4	= htonl(src->data_size4);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->n_hd_sec	= ntohl(src->n_hd_sec);
    dest->info_version	= ntohl(src->info_version);

    dest->itime		= ntoh64(src->itime);
    dest->mtime		= ntoh64(src->mtime);
    dest->ctime		= ntoh64(src->ctime);
    dest->atime		= ntoh64(src->atime);
    dest->dtime		= ntoh64(src->dtime);
}

//-----------------------------------------------------------------------------

void hton_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src )
{
    ASSERT(dest);
    if (!src)
	src = dest;

    dest->n_hd_sec	= htonl(src->n_hd_sec);
    dest->info_version	= htonl(src->info_version);

    dest->itime		= hton64(src->itime);
    dest->mtime		= hton64(src->mtime);
    dest->ctime		= hton64(src->ctime);
    dest->atime		= hton64(src->atime);
    dest->dtime		= hton64(src->dtime);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_ticket_t		///////////////
///////////////////////////////////////////////////////////////////////////////

const char not_encrypted_marker[] = "*** partition is not encrypted ***";

///////////////////////////////////////////////////////////////////////////////

void ticket_clear_encryption ( wd_ticket_t * tik, int mark_not_encrypted )
{
    ASSERT(tik);

    memset(tik->sig,0,sizeof(tik->sig));
    memset(tik->sig_padding,0,sizeof(tik->sig_padding));
    memset(tik->trucha_pad,0,sizeof(tik->trucha_pad));

    if (mark_not_encrypted)
    {
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->sig_padding));
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->trucha_pad));
	strncpy( (char*)tik->sig_padding, not_encrypted_marker, sizeof(tik->sig_padding)-1 );
	strncpy( (char*)tik->trucha_pad, not_encrypted_marker, sizeof(tik->trucha_pad)-1 );
    }
}

///////////////////////////////////////////////////////////////////////////////

bool ticket_is_marked_not_encrypted ( const wd_ticket_t * tik )
{
    ASSERT(tik);
    ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->sig_padding));
    ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->trucha_pad));

    return !strncmp( (char*)tik->sig_padding, not_encrypted_marker, sizeof(tik->sig_padding) )
	&& !strncmp( (char*)tik->trucha_pad, not_encrypted_marker, sizeof(tik->trucha_pad) );
}

///////////////////////////////////////////////////////////////////////////////

u32 ticket_sign_trucha ( wd_ticket_t * tik, u32 tik_size )
{
    ASSERT(tik);
    ticket_clear_encryption(tik,0);

    if (!tik_size)  // auto calculation
	tik_size = sizeof(wd_ticket_t);

    // trucha signing

 #ifdef DEBUG
    TRACE("TRUCHA: start brute force\n");
    //TRACE_HEXDUMP16(0,0,tik,tik_size);
 #endif

    u32 val = 0, count = 0;
    u8 hash[WII_HASH_SIZE];
    do
    {
	count++;

	memcpy(tik->trucha_pad,&val,sizeof(val));
	SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, tik_size-WII_TICKET_SIG_OFF, hash );
	if (!*hash)
	    break;
	//TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	val += 197731421; // any odd number

    } while (val);

    TRACE("TRUCHA: success, count=%u\n",count);
    return *hash ? 0 : count;
}

///////////////////////////////////////////////////////////////////////////////

bool ticket_is_trucha_signed ( const wd_ticket_t * tik, u32 tik_size )
{
    ASSERT(tik);
   
    if (!tik_size)  // auto calculation
	tik_size = sizeof(wd_ticket_t);

    int i;
    for ( i = 0; i < sizeof(tik->sig); i++ )
	if (tik->sig[i])
	    return 0;
	    
    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, tik_size-WII_TICKET_SIG_OFF, hash );
    return !*hash;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_tmd_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void tmd_clear_encryption ( wd_tmd_t * tmd, int mark_not_encrypted )
{
    ASSERT(tmd);

    memset(tmd->sig,0,sizeof(tmd->sig));
    memset(tmd->sig_padding,0,sizeof(tmd->sig_padding));
    memset(tmd->reserved,0,sizeof(tmd->reserved));

    if (mark_not_encrypted)
    {
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->sig_padding));
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->reserved));
	strncpy( (char*)tmd->sig_padding, not_encrypted_marker, sizeof(tmd->sig_padding)-1 );
	strncpy( (char*)tmd->reserved, not_encrypted_marker, sizeof(tmd->reserved)-1 );
    }
}

///////////////////////////////////////////////////////////////////////////////

bool tmd_is_marked_not_encrypted ( const wd_tmd_t * tmd )
{
    ASSERT(tmd);
    ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->sig_padding));
    ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->reserved));

    return !strncmp( (char*)tmd->sig_padding, not_encrypted_marker, sizeof(tmd->sig_padding) )
	&& !strncmp( (char*)tmd->reserved, not_encrypted_marker, sizeof(tmd->reserved) );
}

///////////////////////////////////////////////////////////////////////////////

u32 tmd_sign_trucha ( wd_tmd_t * tmd, u32 tmd_size )
{
    ASSERT(tmd);
    tmd_clear_encryption(tmd,0);

    if (!tmd_size)  // auto calculation
	tmd_size = sizeof(wd_tmd_t) + tmd->n_content * sizeof(wd_tmd_content_t);

    // trucha signing

 #ifdef DEBUG
    TRACE("TRUCHA: start brute force\n");
    //TRACE_HEXDUMP16(0,0,tmd,tmd_size);
 #endif

    u32 val = 0, count = 0;
    u8 hash[WII_HASH_SIZE];
    do
    {
	count++;

	memcpy(tmd->reserved,&val,sizeof(val));
	SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
	if (!*hash)
	    break;
	//TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	val += 197731421; // any odd number

    } while (val);

    TRACE("TRUCHA: success, count=%u\n",count);
    return *hash ? 0 : count;
}

///////////////////////////////////////////////////////////////////////////////

bool tmd_is_trucha_signed ( const wd_tmd_t * tmd, u32 tmd_size )
{
    ASSERT(tmd);
   
    if (!tmd_size)  // auto calculation
	tmd_size = sizeof(wd_tmd_t) + tmd->n_content * sizeof(wd_tmd_content_t);

    int i;
    for ( i = 0; i < sizeof(tmd->sig); i++ )
	if (tmd->sig[i])
	    return 0;
	    
    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
    return !*hash;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_part_control_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define ALIGN32(d,a) ((d)+((a)-1)&~(u32)((a)-1))
#define ALIGN64(d,a) ((d)+((a)-1)&~(u64)((a)-1))

///////////////////////////////////////////////////////////////////////////////

static int setup_part_control_helper ( wd_part_control_t * pc )
{
    // tmd, tmd_size, cert, cert_size and h3 must 0 or be valid

    ASSERT(pc);
    TRACE("setup_part_control_helper(%p)\n",pc);

    TRACE(" - PART: %p : %8x + %9zx\n", pc->part_bin, 0, sizeof(pc->part_bin) );

    // setup head

    wd_part_header_t * head = (wd_part_header_t*)pc->part_bin;
    pc->head		= head;
    pc->head_size	= sizeof(wd_part_header_t);
    TRACE(" - HEAD: %p : %8x + %9x\n", pc->head, 0, pc->head_size );

    u8* head_end = pc->part_bin + ALIGN32(sizeof(wd_part_header_t),4);
    u8* part_end = pc->part_bin + sizeof(pc->part_bin);

    // setup tmd

    u8* ptr		= pc->tmd
				? (u8*) pc->tmd
				: pc->part_bin + ALIGN32(sizeof(wd_part_header_t),32);
    pc->tmd		= (wd_tmd_t*)ptr;
    head->tmd_size	= htonl(pc->tmd_size);
    head->tmd_off4	= htonl(ptr-pc->part_bin>>2);
    TRACE(" - TMD:  %p : %8x + %9x\n", pc->tmd, ntohl(head->tmd_off4)<<2, pc->tmd_size );

    // setup tmd_content

    const u32 n_content = pc->tmd_size >= sizeof(wd_tmd_t)
			? ( pc->tmd_size - sizeof(wd_tmd_t) ) / sizeof(wd_tmd_content_t)
			: 0;
    if ( !pc->tmd->n_content || pc->tmd->n_content > n_content )
	pc->tmd->n_content = n_content;

    pc->tmd_content	= n_content ? pc->tmd->content : 0;
    
    TRACE(" - CON0: %p : %8zx + %9zx [N=%u/%u]\n",
		pc->tmd_content,
		(ntohl(head->tmd_off4)<<2) + sizeof(wd_tmd_t),
		sizeof(wd_tmd_content_t), pc->tmd->n_content, n_content );

    // setup cert

    ptr			= pc->cert
				? (u8*) pc->cert
				: (u8*) pc->tmd + ALIGN32(pc->tmd_size,32);
    pc->cert		= ptr;
    head->cert_size	= htonl(pc->cert_size);
    head->cert_off4	= htonl(ptr-pc->part_bin>>2);
    TRACE(" - CERT: %p : %8x + %9x\n", pc->cert, ntohl(head->cert_off4)<<2, pc->cert_size );

    // setup h3

    if (!pc->h3)
	pc->h3		= pc->part_bin + sizeof(pc->part_bin) - WII_H3_SIZE;
    pc->h3_size		= WII_H3_SIZE;
    head->h3_off4	= htonl(pc->h3-pc->part_bin>>2);
    TRACE(" - H3:   %p : %8x + %9x\n", pc->h3, ntohl(head->h3_off4)<<2, WII_H3_SIZE );

    // setup data

    if (!pc->data_off)
	pc->data_off	= sizeof(pc->part_bin);
    head->data_off4	= htonl(pc->data_off>>2);
    head->data_size4	= htonl(pc->data_size>>2);
    TRACE(" - DATA: %p : %8x + %9llx\n",
		pc->part_bin + sizeof(pc->part_bin),
		ntohl(head->data_off4)<<2, pc->data_size );

    // validation check

    pc->is_valid = 0;

    if ( pc->tmd_size < sizeof(wd_tmd_t) + sizeof(wd_tmd_content_t)
	|| pc->cert_size < 0x10
	|| pc->data_off < sizeof(pc->part_bin)
	|| pc->data_size > WII_MAX_PART_SECTORS * (u64)WII_SECTOR_SIZE
    )
	return 1;

    // check that all data is within part_bin
    
    u8 * tmd_beg  = (u8*)pc->tmd;
    u8 * tmd_end  = tmd_beg + pc->tmd_size;
    u8 * cert_beg = (u8*)pc->cert;
    u8 * cert_end = cert_beg + pc->cert_size;
    u8 * h3_beg   = (u8*)pc->h3;
    u8 * h3_end   = h3_beg   + pc->h3_size;

    if (   tmd_beg  < head_end || tmd_end  > part_end
	|| cert_beg < head_end || cert_end > part_end
	|| h3_beg   < head_end || h3_end   > part_end
    )
	return 1;

    // check overlays

    if (   tmd_end  >= cert_beg && tmd_beg  < cert_end	// overlay tmd  <-> cert
	|| tmd_end  >= h3_beg   && tmd_beg  < h3_end	// overlay tmd  <-> h3
	|| cert_end >= h3_beg   && cert_beg < h3_end	// overlay cert <-> h3
    )
	return 1;

    // all seems ok

    pc->is_valid = 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int clear_part_control
	( wd_part_control_t * pc, u32 tmd_size, u32 cert_size, u64 data_size )
{
    ASSERT(pc);
    TRACE("clear_part_control(%p,%x,%x,%llx)\n",pc,tmd_size,cert_size,data_size);

    memset(pc,0,sizeof(*pc));

    pc->tmd_size	= tmd_size;
    pc->cert_size	= cert_size;
    pc->data_size	= ALIGN64(data_size,WII_SECTOR_SIZE);

    return setup_part_control_helper(pc);
}

///////////////////////////////////////////////////////////////////////////////

int setup_part_control ( wd_part_control_t * pc )
{
    ASSERT(pc);
    TRACE("setup_part_control(%p)\n",pc);

    // clear controling data
    memset( pc->part_bin+sizeof(pc->part_bin), 0, +sizeof(*pc)-sizeof(pc->part_bin) );
 
    wd_part_header_t * head = (wd_part_header_t*)pc->part_bin;

    pc->tmd		= (wd_tmd_t*)
			( pc->part_bin + ( ntohl(head->tmd_off4)   << 2 ));
    pc->tmd_size	=		   ntohl(head->tmd_size);
    pc->cert		= pc->part_bin + ( ntohl(head->cert_off4)  << 2 );
    pc->cert_size	=		   ntohl(head->cert_size);
    pc->h3		= pc->part_bin + ( ntohl(head->h3_off4)    << 2 );
    pc->data_off	=	      (u64)ntohl(head->data_off4)  << 2;
    pc->data_size	=	      (u64)ntohl(head->data_size4) << 2;

    return setup_part_control_helper(pc);
}

///////////////////////////////////////////////////////////////////////////////

u32 part_control_sign_trucha ( wd_part_control_t * pc, int calc_h4 )
{
    ASSERT(pc);

    u32 stat = 0;
    if (pc->is_valid)
    {
	// caluclate SHA1 hash 'h4'
	if ( calc_h4 && pc->tmd_content )
	    SHA1( pc->h3, pc->h3_size, pc->tmd_content->hash );

	// trucha signing
	const u32 stat1 = tmd_sign_trucha(pc->tmd,pc->tmd_size);
	const u32 stat2 = ticket_sign_trucha(&pc->head->ticket,0);
	stat = stat1 && stat2 ? stat + stat2 : 0;
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int part_control_is_trucha_signed ( const wd_part_control_t * pc )
{
    ASSERT(pc);
    return pc->is_valid && pc->tmd && tmd_is_trucha_signed(pc->tmd,pc->tmd_size);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			default helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

unsigned char * wbfs_sha1_fake
	( const unsigned char *d, size_t n, unsigned char *md )
{
    memset(md,0,sizeof(*md));
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

