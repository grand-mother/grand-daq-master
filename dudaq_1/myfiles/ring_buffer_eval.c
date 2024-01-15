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
	S_RingBufferEval *p_rbe;
	uint64_t nb_byte;
	uint8_t *p_buf;
	float *p_prob;
	int ret_mtx;

	p_rbe = (S_RingBufferEval*) malloc(sizeof(S_RingBufferEval));
	if (p_rbe == NULL) {
		RBE_error = 1;
		return NULL;
	}
	p_rbe->inext_eval = 0;
	p_rbe->inext_trig = 0;
	p_rbe->inext_write = 0;
	p_rbe->nb_array = nb_array;
	p_rbe->idx_max = nb_array - 1;
	p_rbe->size_buffer = size_buffer;
	p_rbe->nb_eval = 0;
	p_rbe->nb_trig = 0;
	p_rbe->nb_write = nb_array;
	// alloc array of buffer
	nb_byte = (uint64_t) size_buffer * (uint64_t) nb_array;
	p_buf = (uint8_t*) malloc(nb_byte);
	if (p_buf == NULL) {
		RBE_error = 2;
		return NULL;
	}
	p_rbe->a_buffers = p_buf;
	p_prob = (float*) malloc(sizeof(float) * (uint64_t) nb_array);
	if (p_prob == NULL) {
		RBE_error = 3;
		return NULL;
	}
	p_rbe->a_prob = p_prob;
	ret_mtx = pthread_mutex_init(&p_rbe->mutex, NULL);
	if (ret_mtx != 0) {
		RBE_error = 4;
		return NULL;
	}

	return p_rbe;
}

/**
 * \fn uint8_t RBE_delete(S_RingBufferEval**)
 * \brief
 *
 *
 *
 * \param p_rbe
 * \return
 */
void RBE_delete(S_RingBufferEval **pp_rbe) {
	free((*pp_rbe)->a_buffers);
	free((*pp_rbe)->a_prob);
	pthread_mutex_destroy(&((*pp_rbe)->mutex));
	free(*pp_rbe);
	*pp_rbe = NULL;
}

/**
 * \fn void RBE_write(S_RingBufferEval*, void *p_buf)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
void RBE_write(S_RingBufferEval *p_rbe, const void *p_buf) {
	uint32_t idx_eval_buffer = p_rbe->inext_eval;
	/* index (in byte ) where start buffer to evaluate */
	uint32_t idx_eval_byte = ((uint32_t) idx_eval_buffer) * p_rbe->size_buffer;
	memcpy(&p_rbe->a_buffers[idx_eval_byte], p_buf, p_rbe->size_buffer);
	p_rbe->nb_write -= 1;
	p_rbe->nb_eval += 1;
	RBE_inc_modulo(&p_rbe->inext_write, p_rbe->idx_max);
}

/**
 * \fn void RBE_after_write(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
void RBE_after_write(S_RingBufferEval * const p_rbe) {
	pthread_mutex_lock(&p_rbe->mutex);
	p_rbe->nb_write -= 1;
	p_rbe->nb_eval += 1;
	RBE_inc_modulo(&p_rbe->inext_write, p_rbe->idx_max);
	pthread_mutex_unlock(&p_rbe->mutex);
}

/**
 * \fn void RBE_after_eval(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
void RBE_after_eval(S_RingBufferEval *p_rbe) {
	pthread_mutex_lock(&p_rbe->mutex);
	p_rbe->nb_eval -= 1;
	p_rbe->nb_trig += 1;
	RBE_inc_modulo(&p_rbe->inext_eval, p_rbe->idx_max);
	pthread_mutex_unlock(&p_rbe->mutex);
}

/**
 * \fn void RBE_after_trigger(S_RingBufferEval*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
void RBE_after_trigger(S_RingBufferEval *p_rbe) {
	pthread_mutex_lock(&p_rbe->mutex);
	p_rbe->nb_trig -= 1;
	p_rbe->nb_write += 1;
	RBE_inc_modulo(&p_rbe->inext_trig, p_rbe->idx_max);
	pthread_mutex_unlock(&p_rbe->mutex);
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
