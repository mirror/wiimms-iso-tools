
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

#ifndef WIT_CERT_H
#define WIT_CERT_H 1

#define _GNU_SOURCE 1

#include "file-formats.h"

// some info urls:
//  - http://wiibrew.org/wiki/Certificate_chain

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum cert_stat_flags_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum cert_stat_flags_t
{
    CERT_F_BASE_OK	= 0x0100,	// signature base ok
    CERT_F_BASE_INVALID	= 0x0200,	// signature base wrong

    CERT_F_HASH_OK	= 0x0400,	// hash value ok 
    CERT_F_HASH_FAKED	= 0x0800,	// hash value is fake signed
    CERT_F_HASH_FAILED	= 0x1000,	// hash value invalid

    CERT_F_ERROR	= 0x2000,	// error while checking signature

} cert_stat_flags_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum cert_stat_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum cert_stat_t
{
    CERT_SIG_OK			= CERT_F_BASE_OK | CERT_F_HASH_OK,
    CERT_SIG_FAKE_SIGNED	= CERT_F_BASE_OK | CERT_F_HASH_FAKED,
    CERT_HASH_FAILED		= CERT_F_BASE_OK | CERT_F_HASH_FAILED,

    CERT_HASH_OK		= CERT_F_BASE_INVALID | CERT_F_HASH_OK,
    CERT_HASH_FAKE_SIGNED	= CERT_F_BASE_INVALID | CERT_F_HASH_FAKED,
    CERT_SIG_FAILED		= CERT_F_BASE_INVALID | CERT_F_HASH_FAILED,

    CERT_ERR_TYPE_MISSMATCH	= CERT_F_ERROR + 1,
    CERT_ERR_NOT_SUPPORTED,
    CERT_ERR_NOT_FOUND,
    CERT_ERR_INVALID_SIG,

} cert_stat_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct cert_head_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct cert_head_t
{
    u32			sig_type;	// signature type
    u8			sig_data[0];

} __attribute__ ((packed)) cert_head_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct cert_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct cert_data_t
{
    u8			issuer[0x40]; 	// signature issuer
    u32			key_type;	// key type
    char		key_id[0x40];	// id of key
    u32			unknown1;
    u8			public_key[];

} __attribute__ ((packed)) cert_data_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct cert_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct cert_item_t
{
    const cert_head_t	* head;		// pointer to cert head
    const cert_data_t	* data;		// pointer to cert data
    u32			sig_size;	// size of 'head->sig_data'
    u32			key_size;	// size of 'data->public_key'
    u32			data_size;	// size of 'data'
    u32			cert_size;	// total size of cert

} cert_item_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct cert_chain_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct cert_chain_t
{
    cert_item_t		* cert;		// pointer to certificate list
    int			used;		// used elements of 'cert'
    int			size;		// alloced elements of 'cert' 
        
} cert_chain_t;

extern cert_chain_t global_cert;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: cert helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_message
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
);

///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_name
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
);

///////////////////////////////////////////////////////////////////////////////

ccp cert_get_signature_name
(
    u32			sig_type,	// signature type
    ccp			ret_invalid	// return value if 'sig_type' unknown
);

///////////////////////////////////////////////////////////////////////////////

int cert_get_signature_size // returns NULL for unknown 'sig_type'
(
    u32			sig_type	// signature type
);

///////////////////////////////////////////////////////////////////////////////

int cert_get_pubkey_size // returns NULL for unknown 'sig_type'
(
    u32			key_type	// signature type
);

///////////////////////////////////////////////////////////////////////////////

cert_data_t * cert_get_data // return NULL if invalid
(
    const void		* head		// NULL or pointer to cert header (cert_head_t)
);

///////////////////////////////////////////////////////////////////////////////

cert_head_t * cert_get_next_head // return NULL if invalid
(
    const void		* data		// NULL or pointer to cert data (cert_data_t)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: certificate chain	///////////////
///////////////////////////////////////////////////////////////////////////////

cert_chain_t * cert_initialize
(
    cert_chain_t	* cc		// NULL or pointer to structure
					// if NULL: alloc data
);

///////////////////////////////////////////////////////////////////////////////

void cert_reset
(
    cert_chain_t	* cc		// valid pointer to cert chain
);

///////////////////////////////////////////////////////////////////////////////

cert_item_t * cert_append_item
(
    cert_chain_t	* cc		// valid pointer to cert chain
);

///////////////////////////////////////////////////////////////////////////////

int cert_append_data
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* data,		// NULL or pointer to cert data
    size_t		data_size	// size of 'data'
);

///////////////////////////////////////////////////////////////////////////////

int cert_append_file
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    ccp			filename	// name of file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: check certificate	///////////////
///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* sig_data,	// pointer to signature data
    u32			sig_data_size,	// size of 'sig_data'
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_cert
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const cert_item_t	* item,		// NULL or pointer to certificate data
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_ticket
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_ticket_t	* ticket,	// NULL or pointer to ticket
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_tmd
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_tmd_t	* tmd,		// NULL or pointer to tmd
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_CERT_H
