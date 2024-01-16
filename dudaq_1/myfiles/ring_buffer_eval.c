#include "ring_buffer_eval.h"

#include <stdlib.h>
#include <stdio.h>

int RBE_error = 0;

/**
 * \fn S_RingBufferEval RBE_create*(uint16_t, uint16_t)
 * \brief
 *
 *
 *
 * \param size_buffer
 * \param nb_array
 * \return
 */
S_RingBufferEval*
RBE_create(uint16_t size_buffer, uint16_t nb_array) {
	S_RingBufferEval *self;
	uint64_t nb_byte;
	uint8_t *p_buf;
	float *p_prob;
	int ret_mtx;

	self = (S_RingBufferEval*) malloc(sizeof(S_RingBufferEval));
	if (self == NULL) {
		RBE_error = 1;
		return NULL;
	}
	self->inext_eval = 0;
	self->inext_trig = 0;
	self->inext_write = 0;
	self->nb_array = nb_array;
	self->idx_max = nb_array - 1;
	self->size_buffer = size_buffer;
	self->nb_eval = 0;
	self->nb_trig = 0;
	self->nb_write = nb_array;
	// alloc array of buffer
	nb_byte = (uint64_t) size_buffer * (uint64_t) nb_array;
	p_buf = (uint8_t*) malloc(nb_byte);
	if (p_buf == NULL) {
		RBE_error = 2;
		return NULL;
	}
	self->a_buffers = p_buf;
	p_prob = (float*) malloc(sizeof(float) * (uint64_t) nb_array);
	if (p_prob == NULL) {
		RBE_error = 3;
		return NULL;
	}
	self->a_prob = p_prob;
	ret_mtx = pthread_mutex_init(&self->mutex, NULL);
	if (ret_mtx != 0) {
		RBE_error = 4;
		return NULL;
	}

	return self;
}

/**
 * \fn uint8_t RBE_delete(S_RingBufferEval**)
 * \brief
 *
 *
 *
 * \param self
 * \return
 */
void RBE_delete(S_RingBufferEval **pself) {
	free((*pself)->a_buffers);
	free((*pself)->a_prob);
	pthread_mutex_destroy(&((*pself)->mutex));
	free(*pself);
	*pself = NULL;
}

/**
 * \fn void RBE_write(S_RingBufferEval*, void *p_buf)
 * \brief
 *
 *
 *
 * \param self
 */
void RBE_write(S_RingBufferEval *self, const void *p_buf) {
	uint32_t idx_eval_buffer = self->inext_eval;
	/* index (in byte ) where start buffer to evaluate */
	uint32_t idx_eval_byte = ((uint32_t) idx_eval_buffer) * self->size_buffer;
	memcpy(&self->a_buffers[idx_eval_byte], p_buf, self->size_buffer);
	self->nb_write -= 1;
	self->nb_eval += 1;
	RBE_inc_modulo(&self->inext_write, self->idx_max);
}

/**
 * \fn void RBE_after_write(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param self
 */
void RBE_after_write(S_RingBufferEval * const self) {
	pthread_mutex_lock(&self->mutex);
	self->nb_write -= 1;
	self->nb_eval += 1;
	RBE_inc_modulo(&self->inext_write, self->idx_max);
	pthread_mutex_unlock(&self->mutex);
}

/**
 * \fn void RBE_after_eval(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param self
 */
void RBE_after_eval(S_RingBufferEval *self) {
	pthread_mutex_lock(&self->mutex);
	self->nb_eval -= 1;
	self->nb_trig += 1;
	RBE_inc_modulo(&self->inext_eval, self->idx_max);
	pthread_mutex_unlock(&self->mutex);
}

/**
 * \fn void RBE_after_trigger(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param self
 */
void RBE_after_trigger(S_RingBufferEval *self) {
	pthread_mutex_lock(&self->mutex);
	self->nb_trig -= 1;
	self->nb_write += 1;
	RBE_inc_modulo(&self->inext_trig, self->idx_max);
	pthread_mutex_unlock(&self->mutex);
}

/**
 * \fn uint16_t RBE_inc_modulo(uint16_t*)
 * \brief
 *
 *
 * \param p_int
 * \return
 */

void RBE_inc_modulo(uint16_t* const p_int, uint16_t max_int) {
	/*printf("\n%d",max_int);*/
	if (*p_int == max_int) {
		*p_int = 0;
	} else {
		*p_int += 1;
	}
}

void RBE_print_error(void) {
	printf("\nRFE module : error %d (O is OK)", RBE_error);
}
