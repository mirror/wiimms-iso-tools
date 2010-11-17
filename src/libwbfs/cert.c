
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
#include "cert.h"

#define ALIGN32(d,a) ((d)+((a)-1)&~(u32)((a)-1))

//
///////////////////////////////////////////////////////////////////////////////
///////////////			bn helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

#define bn_zero(a,b)      memset(a,0,b)
#define bn_copy(a,b,c)    memcpy(a,b,c)
#define bn_compare(a,b,c) memcmp(a,b,c)

///////////////////////////////////////////////////////////////////////////////
// calc a = a mod N, given n = size of a,N in bytes

static void bn_sub_modulus ( u8 *a, const u8 *N, const u32 n )
{
    u32 i;
    u32 dig;
    u8 c;

    c = 0;
    for (i = n - 1; i < n; i--) {
	dig = N[i] + c;
	c = (a[i] < dig);
	a[i] -= dig;
    }
}

///////////////////////////////////////////////////////////////////////////////
// calc d = (a + b) mod N, given n = size of d,a,b,N in bytes

static void bn_add ( u8 *d, const u8 *a, const u8 *b, const u8 *N, const u32 n )
{
    u32 i;
    u32 dig;
    u8 c;

    c = 0;
    for (i = n - 1; i < n; i--)
    {
	dig = a[i] + b[i] + c;
	c = (dig >= 0x100);
	d[i] = dig;
    }

    if (c)
	bn_sub_modulus(d, N, n);

    if (bn_compare(d, N, n) >= 0)
	bn_sub_modulus(d, N, n);
}

///////////////////////////////////////////////////////////////////////////////
// calc d = (a * b) mod N, given n = size of d,a,b,N in bytes

static void bn_mul ( u8 *d, const u8 *a, const u8 *b, const u8 *N, const u32 n )
{
    u32 i;
    u8 mask;

    bn_zero(d, n);

    for (i = 0; i < n; i++)
	for (mask = 0x80; mask != 0; mask >>= 1)
	{
	    bn_add(d, d, d, N, n);
	    if ((a[i] & mask) != 0)
		bn_add(d, d, b, N, n);
	}
}

///////////////////////////////////////////////////////////////////////////////
// calc d = (a ^ e) mod N, given n = size of d,a,N and en = size of e in bytes

static void bn_exp(u8 *d, const u8 *a, const u8 *N, const u32 n, const u8 *e, const u32 en)
{
    u8 t[512];
    u32 i;
    u8 mask;

    bn_zero(d, n);
    d[n-1] = 1;
    for (i = 0; i < en; i++)
	for ( mask = 0x80; mask != 0; mask >>= 1 )
	{
	    bn_mul(t, d, d, N, n);
	    if ((e[i] & mask) != 0)
		bn_mul(d, t, a, N, n);
	    else
		bn_copy(d, t, n);
	}
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			cert helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_message
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
)
{
    switch (stat)
    {
	case CERT_SIG_OK:		return "Signature ok";
	case CERT_SIG_FAKE_SIGNED:	return "Signature ok but fake signed";
	case CERT_HASH_FAILED:		return "Signature ok but hash failed";

	case CERT_HASH_OK:		return "Signature wrong but hash is ok";
	case CERT_HASH_FAKE_SIGNED:	return "Signature wrong but fake signed";
	case CERT_SIG_FAILED:		return "Signature wrong";

	case CERT_ERR_TYPE_MISSMATCH:	return "Different types of signature and key";
	case CERT_ERR_NOT_SUPPORTED:	return "This kind of signature is not supported";
	case CERT_ERR_NOT_FOUND:	return "Certificate not found";
	case CERT_ERR_INVALID_SIG:	return "Signature is invalid";
    }

    return ret_invalid;
};
 
///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_name
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
)
{
    switch (stat)
    {
	case CERT_SIG_OK:		return "OK";
	case CERT_SIG_FAKE_SIGNED:	return "SIG FAKE SIGNED";
	case CERT_HASH_FAILED:		return "HASH FAILED";

	case CERT_HASH_OK:		return "HASH OK";
	case CERT_HASH_FAKE_SIGNED:	return "HASH FAKE SIGNED";
	case CERT_SIG_FAILED:		return "SIG FAILED";

	case CERT_ERR_TYPE_MISSMATCH:	return "CERT TYPE MISMATCH";
	case CERT_ERR_NOT_SUPPORTED:	return "CERT TYPE NOT SUPPORTED";
	case CERT_ERR_NOT_FOUND:	return "CERT NOT FOUND";
	case CERT_ERR_INVALID_SIG:	return "SIG INVALID";
    }

    return ret_invalid;
};
 
///////////////////////////////////////////////////////////////////////////////

ccp cert_get_signature_name
(
    u32			sig_type,	// signature type
    ccp			ret_invalid	// return value if 'sig_type' unknown
)
{
    switch (sig_type)
    {
	case 0x10000:	return "RSA-4096";
	case 0x10001:	return "RSA-2048";
	case 0x10002:	return "Elliptic Curve";
    }
    return ret_invalid;
}

///////////////////////////////////////////////////////////////////////////////

int cert_get_signature_size // returns NULL for unknown 'sig_type'
(
    u32			sig_type	// signature type
)
{
    switch (sig_type)
    {
	case 0x10000:	return 0x200;
	case 0x10001:	return 0x100;
	case 0x10002:	return 0x040;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int cert_get_pubkey_size // returns NULL for unknown 'sig_type'
(
    u32			key_type	// signature type
)
{
    return cert_get_signature_size(key_type|0x10000);
}

///////////////////////////////////////////////////////////////////////////////

cert_data_t * cert_get_data // return NULL if invalid
(
    const void		* head		// NULL or pointer to cert header (cert_head_t)
)
{
    cert_data_t * data = 0;
    if (head)
    {
	const u32 sig_size = cert_get_signature_size(be32(head));
	if (sig_size)
	{
	    const u32 head_size = ALIGN32( sig_size + sizeof(cert_head_t), WII_CERT_ALIGN );
	    data = (cert_data_t*)( (u8*)head + head_size );
	}
    }
    return data;
}

///////////////////////////////////////////////////////////////////////////////

cert_head_t * cert_get_next_head // return NULL if invalid
(
    const void		* data		// NULL or pointer to cert data (cert_data_t)
)
{
    cert_head_t * head = 0;
    if (data)
    {
	const cert_data_t * cdata = data;
	const int key_size = cert_get_pubkey_size(ntohl(cdata->key_type));
	if (key_size)
	{
	    const u32 data_size = ALIGN32( key_size + sizeof(cert_data_t), WII_CERT_ALIGN );
	    head = (cert_head_t*)( (u8*)data + data_size );
	}
    }
    return head;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			root cert			///////////////
///////////////////////////////////////////////////////////////////////////////

cert_data_t root_cert =
{
    "",		// issuer
    0,		// key_type
    "Root",	// key_id
    0,		// unknown1
 {
    //--- public_key

    0x24,0xf8,0xb0,0x84, 0x66,0x3b,0x8c,0xdf, 0xdd,0x27,0x6b,0x1e, 0x37,0x3c,0xdd,0xd9,
    0xad,0x06,0x4e,0xff, 0xa4,0x2c,0x8d,0x92, 0x1c,0xdf,0xc1,0x0c, 0x0e,0xc2,0x0f,0x0c,
    0xa2,0x20,0x59,0xfc, 0xb5,0x69,0x02,0x47, 0x65,0x8d,0x74,0x60, 0x4c,0x7e,0x98,0x4e,
    0xb1,0xeb,0x4e,0x49, 0x72,0x48,0xea,0x76, 0x7a,0x7f,0xde,0x8d, 0xd0,0xa7,0xc1,0x31,
    0x09,0x27,0xfc,0x5a, 0x41,0xa3,0xec,0xca, 0x2a,0x62,0xb9,0x0f, 0x5f,0x7d,0xb1,0x6f,
    0xee,0xc7,0x49,0xe9, 0xc4,0x4c,0x6d,0xac, 0xde,0x4f,0xa2,0x3d, 0x4f,0x29,0xa2,0x45,
    0x7e,0x9b,0x92,0x41, 0xe4,0xf8,0x1b,0x72, 0x3f,0x59,0x9d,0x29, 0xbb,0x3b,0x8d,0x50,
    0xa6,0xd2,0xe4,0x3b, 0x37,0x73,0x9d,0xc5, 0xc7,0x13,0x2d,0xa7, 0x9e,0x7a,0x68,0x31,

    0x3a,0x12,0x51,0x3b, 0xed,0x53,0xa3,0x8e, 0xd8,0x6f,0x45,0xd2, 0xfe,0xbb,0x99,0x73,
    0x08,0x59,0x6e,0x98, 0x4f,0xdc,0x57,0xd4, 0x1b,0x2a,0x6b,0x39, 0xb7,0xde,0x6f,0x34,
    0x22,0xd0,0x41,0x59, 0x40,0x64,0x6a,0x5e, 0xff,0x64,0x77,0xfb, 0x32,0x83,0xb9,0xe4,
    0xdb,0x57,0xf1,0x65, 0xc2,0xf6,0xc9,0xe2, 0x59,0x5d,0x5c,0xae, 0x7e,0xe7,0xb1,0x05,
    0xee,0x5d,0xd9,0x93, 0xb3,0x6c,0x2a,0x29, 0x71,0xf4,0xe2,0x16, 0xd7,0xa6,0x2f,0x88,
    0x89,0x3c,0xe1,0x7b, 0x6a,0x5f,0xfa,0x2f, 0x30,0x5f,0x96,0x2f, 0xc8,0xd8,0x56,0x1a,
    0x03,0xfc,0x0e,0x59, 0xd4,0xbb,0xe0,0x77, 0xbe,0x7e,0x1b,0x60, 0xcf,0xc6,0x8f,0xe2,
    0xd7,0xba,0x5c,0xb7, 0xc0,0xec,0xba,0x97, 0xeb,0xff,0xed,0x61, 0x18,0x6c,0x16,0x04,

    0x0d,0xc2,0x3b,0x67, 0x05,0xf4,0x89,0x94, 0x76,0x30,0xc3,0xba, 0x34,0xfd,0x6f,0x14,
    0x7c,0x9b,0xb5,0xdc, 0x19,0x3a,0x54,0x34, 0xd0,0x12,0xe0,0xbd, 0x0a,0x40,0x67,0x7d,
    0xeb,0x1a,0xbc,0x93, 0xa6,0xae,0x01,0x50, 0xa7,0xe2,0xe1,0x8d, 0xf5,0xd1,0x76,0xb6,
    0x85,0xa7,0xd4,0xc3, 0x41,0xea,0xef,0x7f, 0x9a,0xa6,0xe9,0xbd, 0xd5,0x70,0x7b,0x01,
    0xa1,0xf2,0xf3,0x6e, 0x1d,0x72,0x64,0x3e, 0xd3,0x94,0x4e,0x04, 0x65,0x24,0x68,0xb3,
    0x92,0xe0,0xcd,0x28, 0x28,0xa1,0x57,0xa9, 0xa1,0x22,0x22,0x7f, 0x55,0x40,0xef,0x85,
    0x80,0x82,0x21,0x37, 0x17,0x77,0x34,0x9d, 0xe2,0xe6,0x46,0x5c, 0xe0,0xb5,0xe9,0xb2,
    0x6e,0x6e,0x71,0x80, 0x18,0x14,0x84,0x99, 0x82,0x29,0x2b,0x6f, 0xda,0x98,0x68,0xa0,

    0xb8,0xda,0x50,0x03, 0x5c,0x43,0xaa,0xde, 0x86,0xf1,0x68,0x9a, 0x3c,0xe1,0xa0,0x2a,
    0xf3,0xe8,0x3b,0xde, 0x99,0xa7,0xde,0x78, 0x13,0x81,0x41,0x09, 0xe0,0x79,0xe6,0xa0,
    0x7a,0xf5,0xa4,0x50, 0xbb,0x16,0xd4,0x63, 0x30,0x16,0x9f,0x75, 0x8b,0x71,0xca,0x15,
    0x92,0xc0,0x04,0xa9, 0x16,0xcc,0xa1,0x12, 0xa2,0xdd,0xc4,0x2c, 0x03,0xb7,0x22,0x39,
    0xc1,0x07,0x05,0x4d, 0x1e,0xb2,0xbc,0x11, 0x94,0x84,0x76,0x85, 0xf0,0x5e,0xdc,0xa9,
    0x2e,0x43,0x8e,0xb0, 0x4d,0xa0,0xb3,0x39, 0x9c,0xe2,0x7b,0x08, 0x79,0xd0,0x30,0xe7,
    0xaf,0x58,0x02,0x54, 0xb2,0x5e,0x0e,0x37, 0x91,0x92,0x9e,0x69, 0x2e,0x6d,0x95,0x74,
    0xc2,0x7b,0x12,0xad, 0x98,0x00,0xf5,0x48, 0x13,0x18,0x92,0xc3, 0x4d,0x17,0x08,0x49,

    //---- post data
    0x00,0x01,0x00,0x01,
  }
};

///////////////////////////////////////////////////////////////////////////////

cert_item_t * cert_append_item
(
    cert_chain_t	* cc		// valid pointer to cert chain
)
{
    DASSERT(cc);
    if ( cc->used == cc->size )
    {
	cc->size = 2 * cc->size + 5;
	cc->cert = realloc(cc->cert,cc->size*sizeof(*cc->cert));
	if (!cc->cert)
	    OUT_OF_MEMORY;
    }

    cert_item_t * item = cc->cert + cc->used++;
    memset(item,0,sizeof(*item));
    return item;
}

///////////////////////////////////////////////////////////////////////////////

static void cert_add_root()
{
    static bool done = false;
    if (!done)
    {
	done = true;

	cert_item_t * item = cert_append_item(&global_cert);
	DASSERT(item);

	item->data	= &root_cert;
	item->key_size	= cert_get_pubkey_size(ntohl(root_cert.key_type));
	item->data_size	= sizeof(root_cert);
	item->cert_size	= sizeof(root_cert);

	int i;
	for ( i = 0; i < item->key_size; i++ )
	    root_cert.public_key[i] ^= 0xdc;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			certificate chain		///////////////
///////////////////////////////////////////////////////////////////////////////

cert_chain_t global_cert = {0};

///////////////////////////////////////////////////////////////////////////////

cert_chain_t * cert_initialize
(
    cert_chain_t	* cc		// NULL or pointer to structure
					// if NULL: alloc data
)
{
    if (!cc)
    {
	cc = malloc(sizeof(*cc));
	if (!cc)
	    OUT_OF_MEMORY;
    }
    
    memset(cc,0,sizeof(*cc));
    return cc;
}

///////////////////////////////////////////////////////////////////////////////

void cert_reset
(
    cert_chain_t	* cc		// valid pointer to cert chain
)
{
    DASSERT(cc);
    int i;
    for ( i = 0; i < cc->used; i++ )
	free((void*)cc->cert[i].head);
    free(cc->cert);
    cert_initialize(cc);
}

///////////////////////////////////////////////////////////////////////////////

int cert_append_data
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* data,		// NULL or pointer to cert data
    size_t		data_size	// size of 'data'
)
{
    DASSERT(cc);
    TRACE("cert_append_data(%p,%p,%zu) n=%u/%u\n",
		cc, data, data_size, cc->used, cc->size );

    if ( cc == &global_cert )
	cert_add_root();

    const int start_count = cc->used;
    if ( data && data_size > 0 )
    {
	//--- analyze cert data

	const u8 * ptr = data;
	const u8 * end = ptr + data_size;
	while ( ptr < end )
	{
	    // [2do]
	    const u8 * start = ptr;
	    const cert_head_t * head = (cert_head_t*)ptr;
	    const cert_data_t * data = cert_get_data(head);
	    if (!data)
		break;
	    ptr = (u8*)cert_get_next_head(data);
	    if ( !ptr || ptr > end )
		break;

	    cert_item_t * item	= cert_append_item(cc);
	    DASSERT(item);
	    item->sig_size	= cert_get_signature_size(ntohl(head->sig_type));
	    item->key_size	= cert_get_pubkey_size(ntohl(data->key_type));
	    item->data_size	= ptr - (u8*)data;
	    item->cert_size	= ptr - start;

	    const u8 * raw	= MemDup(start,item->cert_size);
	    item->head		= (cert_head_t*)raw;
	    item->data		= (cert_data_t*)( raw + ( (u8*)data - start ));

	    TRACE("s=%04x k=%04x : d=%04x c=%04x : t=%08x,%08x : %s . %s\n",
			item->sig_size, item->key_size,
			item->data_size, item->cert_size,
			ntohl(head->sig_type), ntohl(data->key_type),
			data->issuer, data->key_id );
	}
    }

    TRACE("N-CERT=%u->%u/%u\n",start_count,cc->used,cc->size);
    return cc->used - start_count;
}

///////////////////////////////////////////////////////////////////////////////

int cert_append_file
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    ccp			filename	// name of file
)
{
    DASSERT(cc);
    char buf[0x10000];

    FILE * f = fopen(filename,"rb");
    if (!f)
	return -1;

    size_t stat = fread(buf,1,sizeof(buf),f);
    fclose(f);
    return stat ? cert_append_data(cc,buf,stat) : 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			check certificate		///////////////
///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* sig_data,	// pointer to signature data
    u32			sig_data_size,	// size of 'sig_data'
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
)
{
    TRACE("\n--- cert_check(%p,%p,%x=%u)\n", cc, sig_data, sig_data_size, sig_data_size );

    if (cert_found)
	*cert_found = 0;


    //--- is sig valid?

    if ( !sig_data || sig_data_size <= WII_CERT_ALIGN )
	return CERT_ERR_INVALID_SIG;

    if (!cert_get_signature_size(be32(sig_data)))
	return CERT_ERR_NOT_SUPPORTED;

    //HEXDUMP16(0,0,sig_data,16);
    const cert_head_t * head = sig_data;
    const cert_data_t * data = cert_get_data(sig_data);
    if (!data)
	return CERT_ERR_INVALID_SIG;

    const int test_data_size = sig_data_size - ( (u8*)data - (u8*)head );
    if ( test_data_size <= WII_CERT_ALIGN )
	return 0;

    TRACE("VALID: %s . %s\n", data->issuer, data->key_id );


    //--- find parent key in chain

    int base_len;
    ccp parent_id = strrchr((ccp)data->issuer,'-');
    if (parent_id)
    {
	base_len = parent_id - (ccp)data->issuer;
	parent_id++;
    }
    else
    {
	base_len = 0;
	parent_id = (ccp)data->issuer;
    }

    for(;;)
    {    
	if ( cc == &global_cert )
	    cert_add_root();

	int c;
	for ( c = 0; c < cc->used; c++ )
	{
	    cert_item_t * item = cc->cert + c;
	    if (!strcmp(parent_id,(ccp)item->data->key_id)
		&& ( !base_len || !memcmp(data->issuer,item->data->issuer,base_len)
				    && !item->data->issuer[base_len] ))
	    {
		TRACE("FOUND: %s . %s [%s]\n",
			    item->data->issuer, item->data->key_id,
			    item->head
				? cert_get_signature_name(ntohl(item->head->sig_type),"?")
				: "?" );
		if (cert_found)
		    *cert_found = item;

		const u32 key_type = ntohl(item->data->key_type);
		const u32 sig_type = ntohl(head->sig_type);
		noPRINT("%x %x %x\n",key_type,key_type|0x10000,sig_type);
		if ( (key_type|0x10000) != sig_type )
		    return CERT_ERR_TYPE_MISSMATCH;

		u8 hash[WII_HASH_SIZE];
		SHA1((u8*)data,test_data_size,hash);
		//PRINT("HASH: "); HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);

		u8 buf[0x200];
		const int sig_len = cert_get_signature_size(sig_type);
		if ( sig_len > sizeof(buf) )
		    return CERT_ERR_TYPE_MISSMATCH;
		const u8 * key = item->data->public_key;
		bn_exp(buf,head->sig_data,key,sig_len,key+sig_len,4);
		//HEXDUMP16(0,0,buf,sig_len);

		static const u8 ber[16] = { 0x00,0x30,0x21,0x30,0x09,0x06,0x05,0x2b,
					    0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14 };
		const int ber_index = sig_len - 36;
		const u8 * h = buf + sig_len - WII_HASH_SIZE;

		if (   buf[0] == 0x00
		    && buf[1] == 0x01
		    && buf[2] == 0xff
		    && !memcmp(buf+2,buf+3,sig_len-ber_index-3)
		    && !memcmp(buf+ber_index,ber,sizeof(ber))
		)
		{
		    return !memcmp(h,hash,WII_HASH_SIZE)
			    ? CERT_SIG_OK
			    : !strncmp((ccp)h,(ccp)hash,WII_HASH_SIZE)
				    ? CERT_SIG_FAKE_SIGNED
				    : CERT_HASH_FAILED;
		}
		return !memcmp(h,hash,WII_HASH_SIZE)
			? CERT_HASH_OK
			: !strncmp((ccp)h,(ccp)hash,WII_HASH_SIZE)
				? CERT_HASH_FAKE_SIGNED
				: CERT_SIG_FAILED;
	    }
	}
	if ( cc == &global_cert )
	    return CERT_ERR_NOT_FOUND;
	cc = &global_cert;
    }
}

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_cert
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const cert_item_t	* item,		// NULL or pointer to certificate data
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
)
{
    if ( !item || !item->head )
    {
	if (cert_found)
	    *cert_found = 0;
	return CERT_ERR_INVALID_SIG;
    }

    return cert_check(cc,item->head,item->cert_size,cert_found);
};

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_ticket
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_ticket_t	* ticket,	// NULL or pointer to ticket
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
)
{
    if (!ticket)
    {
	if (cert_found)
	    *cert_found = 0;
	return CERT_ERR_INVALID_SIG;
    }

    return cert_check(cc,ticket,sizeof(*ticket),cert_found);
};

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_tmd
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_tmd_t	* tmd,		// NULL or pointer to tmd
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
)
{
    if (!tmd)
    {
	if (cert_found)
	    *cert_found = 0;
	return CERT_ERR_INVALID_SIG;
    }

    const int tmd_size	= sizeof(wd_tmd_t)
			+ ntohs(tmd->n_content) * sizeof(wd_tmd_content_t);
    return cert_check(cc,tmd,tmd_size,cert_found);
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

