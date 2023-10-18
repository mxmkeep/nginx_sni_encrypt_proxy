/**
 * \file lib/rijndael.h
 *
 * \brief rijndael - An implementation of the Rijndael cipher.
 */

/*
 * Copyright (C) 2000, 2001 Rafael R. Sevilla <sevillar@team.ph.inter.net>
 *
 * Currently maintained by brian d foy, <bdfoy@cpan.org>
 *
 *  License (GNU General Public License):
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *****************************************************************************
*/
/*
 * Rijndael is a 128/192/256-bit block cipher that accepts key sizes of
 * 128, 192, or 256 bits, designed by Joan Daemen and Vincent Rijmen.  See
 * http://www.esat.kuleuven.ac.be/~rijmen/rijndael/ for details.
 */
#ifndef RIJNDAEL_H
#define RIJNDAEL_H 1

#define RIJNDAEL_BLOCKSIZE 16

size_t
rij_encrypt(unsigned char *in, size_t in_len,
    ngx_str_t* keyb64,
    unsigned char *out, ngx_pool_t* pool);
size_t
rij_decrypt(unsigned char *in, size_t in_len,
    ngx_str_t* keyb64,
    unsigned char *out, ngx_pool_t* pool);



#endif /* RIJNDAEL_H */
