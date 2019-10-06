/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu  <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#ifndef UK_BLKREQ_H_
#define UK_BLKREQ_H_

#include <uk/arch/types.h>

/**
 * Unikraft block API request declaration.
 *
 * This header contains all the API data types used for requests operation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define __sector size_t
struct uk_blkreq;

/**
 *	Operation status
 */
enum uk_blkreq_state {
	UK_BLKDEV_REQ_FINISHED = 0,
	UK_BLKDEV_REQ_UNFINISHED
};

/**
 * Supported operations
 */
enum uk_blkreq_op {
	/* Read operation */
	UK_BLKDEV_READ = 0,
	/* Write operation */
	UK_BLKDEV_WRITE,
	/* Flush the volatile write cache */
	UK_BLKDEV_FFLUSH = 4
};

/**
 * Function type used for request callback after a response is processed.
 *
 * @param req
 *	The request object on which the event is triggered
 * @param cookie_callback
 *	Optional parameter set by user at request submit.
 */
typedef void (*uk_blkreq_event_t)(struct uk_blkreq *req, void *cb_cookie);

/**
 * Used for sending a request to the device.
 */
struct uk_blkreq {
	/* Input members */
	/* Operation type */
	enum uk_blkreq_op			operation;
	/* Start Sector from where the op begin */
	__sector				start_sector;
	/* Size in number of sectors */
	__sector				nb_sectors;
	/* Pointer to data */
	void					*aio_buf;
	/* Request callback and its parameters */
	uk_blkreq_event_t			cb;
	void					*cb_cookie;

	/* Output members */
	/* State of request: finished/unfinished*/
	__atomic				state;
	/* Result status of operation (< 0 on errors)*/
	int					result;

};

/**
 * Initializes a request structure.
 *
 * @param req
 *	The request structure
 * @param op
 *	The operation
 * @param start
 *	The start sector
 * @param nb_sectors
 *	Number of sectors
 * @param aio_buf
 *	Data buffer
 * @param cb
 *	Request callback
 * @param cb_cookie
 *	Request callback parameters
 **/
static inline void uk_blkreq_init(struct uk_blkreq *req,
		enum uk_blkreq_op op, __sector start, __sector nb_sectors,
		void *aio_buf, uk_blkreq_event_t cb, void *cb_cookie)
{
	req->operation = op;
	req->start_sector = start;
	req->nb_sectors = nb_sectors;
	req->aio_buf = aio_buf;
	ukarch_store_n(&req->state.counter, UK_BLKDEV_REQ_UNFINISHED);
	req->cb = cb;
	req->cb_cookie = cb_cookie;
}

/**
 * Checks if request is finished.
 *
 * @param req
 *	uk_blkreq structure
 **/
#define uk_blkreq_is_done(req) \
		(ukarch_load_n(&(req)->state.counter) == UK_BLKDEV_REQ_FINISHED)

#ifdef __cplusplus
}
#endif

#endif /* UK_BLKREQ_H_ */
