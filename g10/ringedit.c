/* ringedit.c -  Function for key ring editing
 *	Copyright (c) 1997 by Werner Koch (dd9jn)
 *
 * This file is part of G10.
 *
 * G10 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * G10 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/****************
 * This module supplies function for:
 *
 *  - Search for a key block (pubkey and all other stuff) and return a
 *    handle for it.
 *
 *  - Lock/Unlock a key block
 *
 *  - Read a key block into a tree
 *
 *  - Update a key block
 *
 *  - Insert a new key block
 *
 *  - Delete a key block
 *
 * FIXME:  Add backup stuff
 * FIXME:  Keep track of all nodes, so that a change is propagated
 *	   to all nodes. (or use shallow copies and ref-counting?)
 */



#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "packet.h"
#include "memory.h"
#include "mpi.h"
#include "iobuf.h"
#include "keydb.h"


struct resource_table_struct {
    int used;
    char *fname;
    IOBUF iobuf;
};
typedef struct resource_table_struct RESTBL;

#define MAX_RESOURCES 10
static RESTBL resource_table[MAX_RESOURCES];


static int keyring_search( PACKET *pkt, KBPOS *kbpos, IOBUF iobuf );
static int keyring_read( KBPOS *kbpos, KBNODE *ret_root );
static int keyring_insert( KBPOS *kbpos, KBNODE root );
static int keyring_delete( KBPOS *kbpos );



static RESTBL *
check_pos( KBPOS *kbpos )
{
    if( kbpos->resno < 0 || kbpos->resno >= MAX_RESOURCES )
	return NULL;
    if( !resource_table[kbpos->resno].used )
	return NULL;
    return resource_table + kbpos->resno;
}



/****************************************************************
 ****************** public functions ****************************
 ****************************************************************/

/****************
 * Register a resource (which currently may ionly be a keyring file).
 */
int
add_keyblock_resource( const char *filename, int force )
{
    IOBUF iobuf;
    int i;

    for(i=0; i < MAX_RESOURCES; i++ )
	if( !resource_table[i].used )
	    break;
    if( i == MAX_RESOURCES )
	return G10ERR_RESOURCE_LIMIT;

    iobuf = iobuf_open( filename );
    if( !iobuf && !force )
	return G10ERR_OPEN_FILE;
    resource_table[i].used = 1;
    resource_table[i].fname = m_strdup(filename);
    resource_table[i].iobuf = iobuf;
    return 0;
}


/****************
 * Get a keyblock handle KBPOS from a filename. This can be used
 * to get a handle for insert_keyblock for a new keyblock.
 */
int
get_keyblock_handle( const char *filename, KBPOS *kbpos )
{
    int i;

    for(i=0; i < MAX_RESOURCES; i++ )
	if( resource_table[i].used ) {
	    /* fixme: dos needs case insensitive file compare */
	    if( !strcmp( resource_table[i].fname, filename ) ) {
		memset( kbpos, 0, sizeof *kbpos );
		kbpos->resno = i;
		return 0;
	    }
	}
    return -1; /* not found */
}

/****************
 * Search a keyblock which starts with the given packet and put all
 * informations into KBPOS, which can be used later to access this key block.
 * This function looks into all registered keyblock sources.
 * PACKET must be a packet with either a secret_cert or a public_cert
 *
 * This function is intended to check wether a given certificate
 * is already in a keyring or to prepare it for editing.
 *
 * Returns: 0 if found, -1 if not found or an errorcode.
 */
int
search_keyblock( PACKET *pkt, KBPOS *kbpos )
{
    int i, rc, last_rc=-1;

    for(i=0; i < MAX_RESOURCES; i++ ) {
	if( resource_table[i].used ) {
	    /* note: here we have to add different search functions,
	     * depending on the type of the resource */
	    rc = keyring_search( pkt, kbpos, resource_table[i].iobuf );
	    if( !rc ) {
		kbpos->resno = i;
		return 0;
	    }
	    if( rc != -1 ) {
		log_error("error searching resource %d: %s\n",
						  i, g10_errstr(rc));
		last_rc = rc;
	    }
	}
    }
    return last_rc;
}


/****************
 * Combined function to search for a username and get the position
 * of the keyblock.
 */
int
search_keyblock_byname( KBPOS *kbpos, const char *username )
{
    PACKET pkt;
    PKT_public_cert *pkc = m_alloc_clear( sizeof *pkc );
    int rc;

    rc = get_pubkey_byname( pkc, username );
    if( rc ) {
	free_public_cert(pkc);
	return rc;
    }

    init_packet( &pkt );
    pkt.pkttype = PKT_PUBLIC_CERT;
    pkt.pkt.public_cert = pkc;
    rc = search_keyblock( &pkt, kbpos );
    free_public_cert(pkc);
    return rc;
}


/****************
 * Lock the keyblock; wait until it's available
 * This function may change the internal data in kbpos, in cases
 * when the to be locked keyblock has been modified.
 * fixme: remove this function and add an option to search_keyblock()?
 */
int
lock_keyblock( KBPOS *kbpos )
{
    int rc;

    if( !check_pos(kbpos) )
	return G10ERR_GENERAL;
    return 0;
}

/****************
 * Release a lock on a keyblock
 */
void
unlock_keyblock( KBPOS *kbpos )
{
    if( !check_pos(kbpos) )
	log_bug(NULL);
}

/****************
 * Read a complete keyblock and return the root in ret_root.
 */
int
read_keyblock( KBPOS *kbpos, KBNODE *ret_root )
{
    if( !check_pos(kbpos) )
	return G10ERR_GENERAL;
    return keyring_read( kbpos, ret_root );
}

/****************
 * Insert the keyblock described by ROOT into the keyring described
 * by KBPOS.  This actually appends the data to the keyfile.
 */
int
insert_keyblock( KBPOS *kbpos, KBNODE root )
{
    int rc;

    if( !check_pos(kbpos) )
	return G10ERR_GENERAL;

    rc = keyring_insert( kbpos, root );

    return rc;
}

/****************
 * Delete the keyblock described by KBPOS.
 * The current code simply changes the keyblock in the keyring
 * to packet of type 0 with the correct length.  To help detecting errors,
 * zero bytes are written.
 */
int
delete_keyblock( KBPOS *kbpos )
{
    int rc;

    if( !check_pos(kbpos) )
	return G10ERR_GENERAL;

    rc = keyring_delete( kbpos );

    return rc;
}


/****************
 * Update the keyblock at KBPOS with the one in ROOT.
 */
int
update_keyblock( KBPOS *kbpos, KBNODE root )
{
    int rc;
    KBPOS kbpos2;

    /* we do it the simple way: */
    memset( &kbpos2, 0, sizeof kbpos2 );
    kbpos2.resno = kbpos->resno;
    rc = insert_keyblock( &kbpos2, root );
    if( !rc )
	rc = delete_keyblock( kbpos );

    return rc;
}


/****************************************************************
 ********** Functions which operates on regular keyrings ********
 ****************************************************************/


/****************
 * search one keyring, return 0 if found, -1 if not found or an errorcode.
 */
static int
keyring_search( PACKET *req, KBPOS *kbpos, IOBUF iobuf )
{
    int rc;
    PACKET pkt;
    int save_mode;
    ulong offset;
    int pkttype = req->pkttype;
    PKT_public_cert *req_pkc = req->pkt.public_cert;
    PKT_secret_cert *req_skc = req->pkt.secret_cert;

    init_packet(&pkt);
    save_mode = set_packet_list_mode(0);

    if( iobuf_seek( iobuf, 0 ) ) {
	log_error("can't rewind keyring file: %s\n", g10_errstr(rc));
	rc = G10ERR_KEYRING_OPEN;
	goto leave;
    }

    while( !(rc=search_packet(iobuf, &pkt, pkttype, &offset)) ) {
	if( pkt.pkttype == PKT_SECRET_CERT ) {
	    PKT_secret_cert *skc = pkt.pkt.secret_cert;

	    if(   req_skc->timestamp == skc->timestamp
	       && req_skc->valid_days == skc->valid_days
	       && req_skc->pubkey_algo == skc->pubkey_algo
	       && (   ( skc->pubkey_algo == PUBKEY_ALGO_ELGAMAL
			&& !mpi_cmp( req_skc->d.elg.p, skc->d.elg.p )
			&& !mpi_cmp( req_skc->d.elg.g, skc->d.elg.g )
			&& !mpi_cmp( req_skc->d.elg.y, skc->d.elg.y )
			&& !mpi_cmp( req_skc->d.elg.x, skc->d.elg.x )
		      )
		   || ( skc->pubkey_algo == PUBKEY_ALGO_RSA
			&& !mpi_cmp( req_skc->d.rsa.rsa_n, skc->d.rsa.rsa_n )
			&& !mpi_cmp( req_skc->d.rsa.rsa_e, skc->d.rsa.rsa_e )
			&& !mpi_cmp( req_skc->d.rsa.rsa_d, skc->d.rsa.rsa_d )
		      )
		  )
	      )
		break; /* found */
	}
	else if( pkt.pkttype == PKT_PUBLIC_CERT ) {
	    PKT_public_cert *pkc = pkt.pkt.public_cert;

	    if(   req_pkc->timestamp == pkc->timestamp
	       && req_pkc->valid_days == pkc->valid_days
	       && req_pkc->pubkey_algo == pkc->pubkey_algo
	       && (   ( pkc->pubkey_algo == PUBKEY_ALGO_ELGAMAL
			&& !mpi_cmp( req_pkc->d.elg.p, pkc->d.elg.p )
			&& !mpi_cmp( req_pkc->d.elg.g, pkc->d.elg.g )
			&& !mpi_cmp( req_pkc->d.elg.y, pkc->d.elg.y )
		      )
		   || ( pkc->pubkey_algo == PUBKEY_ALGO_RSA
			&& !mpi_cmp( req_pkc->d.rsa.rsa_n, pkc->d.rsa.rsa_n )
			&& !mpi_cmp( req_pkc->d.rsa.rsa_e, pkc->d.rsa.rsa_e )
		      )
		  )
	      )
		break; /* found */
	}
	else
	    log_bug(NULL);
	free_packet(&pkt);
    }
    if( !rc )
	kbpos->offset = offset;

  leave:
    free_packet(&pkt);
    set_packet_list_mode(save_mode);
    return rc;
}


static int
keyring_read( KBPOS *kbpos, KBNODE *ret_root )
{
    PACKET *pkt;
    int rc;
    RESTBL *rentry;
    KBNODE root = NULL;
    KBNODE node, n1, n2;
    IOBUF a;

    if( !(rentry=check_pos(kbpos)) )
	return G10ERR_GENERAL;

    a = iobuf_open( rentry->fname );
    if( !a ) {
	log_error("can't open '%s'\n", rentry->fname );
	return G10ERR_OPEN_FILE;
    }

    if( iobuf_seek( a, kbpos->offset ) ) {
	log_error("can't seek to %lu: %s\n", kbpos->offset, g10_errstr(rc));
	iobuf_close(a);
	return G10ERR_KEYRING_OPEN;
    }


    pkt = m_alloc( sizeof *pkt );
    init_packet(pkt);
    while( (rc=parse_packet(a, pkt)) != -1 ) {
	if( rc ) {  /* ignore errors */
	    free_packet( pkt );
	    continue;
	}
	switch( pkt->pkttype ) {
	  case PKT_PUBLIC_CERT:
	  case PKT_SECRET_CERT:
	    if( root )
		goto ready;
	    root = new_kbnode( pkt );
	    pkt = m_alloc( sizeof *pkt );
	    init_packet(pkt);
	    break;

	  case PKT_USER_ID:
	    if( !root ) {
		log_error("read_keyblock: orphaned user id\n" );
		rc = G10ERR_INV_KEYRING; /* or wrong kbpos */
		goto ready;
	    }
	    /* append the user id */
	    node = new_kbnode( pkt );
	    if( !(n1=root->child) )
		root->child = node;
	    else {
		for( ; n1->next; n1 = n1->next)
		    ;
		n1->next = node;
	    }
	    pkt = m_alloc( sizeof *pkt );
	    init_packet(pkt);
	    break;

	  case PKT_SIGNATURE:
	    if( !root ) {
		log_error("read_keyblock: no root for signature\n" );
		rc = G10ERR_INV_KEYRING; /* or wrong kbpos */
		break;
	    }
	    if( !root->child ) {
		log_error("read_keyblock: no userid for signature\n" );
		rc = G10ERR_INV_KEYRING;
		break;
	    }
	    /* goto the last user id */
	    for(n1=root->child; n1->next; n1 = n1->next )
		;
	    /* append the signature node */
	    node = new_kbnode( pkt );
	    if( !(n2=n1->child) )
		n1->child = node;
	    else {
		for( ; n2->next; n2 = n2->next)
		    ;
		n2->next = node;
	    }
	    pkt = m_alloc( sizeof *pkt );
	    init_packet(pkt);
	    break;

	  default: /* ignore all other packets. FIXME: we should not do this */
	    free_packet( pkt );
	    break;
	}
    }
  ready:
    kbpos->last_block = rc == -1; /* flag, that this is the last block */
    if( rc == -1 && root )
	rc = 0;

    if( rc )
	release_kbnode( root );
    else {
	*ret_root = root;
	kbpos->length = iobuf_tell( a ) - kbpos->offset;
    }
    free_packet( pkt );
    m_free( pkt );
    iobuf_close(a);
    return rc;
}


/****************
 * Insert the keyblock described by ROOT into the keyring described
 * by KBPOS.  This actually appends the data to the keyfile.
 */
static int
keyring_insert( KBPOS *kbpos, KBNODE root )
{
    RESTBL *rentry;
    IOBUF fp;
    KBNODE kbctx, node;
    int rc;

    if( !(rentry = check_pos( kbpos )) )
	return G10ERR_GENERAL;

    /* FIXME: we must close the file if it's already open, due to
     *	      2 reasons:
     *	       - cannot open the same file twice on DOSish OSes
     *	       - must sync with iobufs somehow
     */
    /* open the file for append */
    fp = iobuf_append( rentry->fname );
    if( !fp ) {
	log_error("can't append to '%s'\n", rentry->fname );
	return G10ERR_OPEN_FILE;
    }

    kbctx=NULL;
    while( (node = walk_kbtree( root, &kbctx )) ) {
	if( (rc = build_packet( fp, node->pkt )) ) {
	    log_error("build_packet(%d) failed: %s\n",
			node->pkt->pkttype, g10_errstr(rc) );
	    return G10ERR_WRITE_FILE;
	}
    }
    iobuf_close(fp);

    return 0;
}

static int
keyring_delete( KBPOS *kbpos )
{
    return -1;
}


/****************************************************************
 ********** Functions which operates on databases ***************
 ****************************************************************/

