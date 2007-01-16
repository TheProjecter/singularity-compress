/*
 * Singularity-compress: statistical model
 * Copyright (C) 2006-2007  Torok Edwin (edwintorok@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef _MODEL_H
#define _MODEL_H

/* keep the blocksize below 1<<16 or you'll see overflows */
#define BLOCKSIZE_POWER 10
#define BLOCKSIZE (1<<BLOCKSIZE_POWER)


#define EXTRA_SYMBOLS 28
#define SYMBOLS (256 + EXTRA_SYMBOLS)

struct ari_model {
	size_t* counts;
};

static void model_setup(struct ari_model* model)
{
	size_t i;
	model->counts = malloc(sizeof(model->counts[0])*(SYMBOLS+1));
	for(i=0;i < SYMBOLS+1;i++) {
			model->counts[i]=i;
	}
}

static void model_done(struct ari_model* model)
{
	free(model->counts);
}

static void model_get_freq(const struct ari_model* model,const uint16_t symbol,freq* cur_freq,freq* cum_freq,freq* total_freq)
{
	*cur_freq = model->counts[symbol+1] - model->counts[symbol];
	*cum_freq = model->counts[symbol];
	*total_freq = model->counts[SYMBOLS];
}

static void model_update_freq(struct ari_model* model,const uint16_t symbol)
{
	size_t i;
	/*fprintf(stderr,"Updating:%d : %ld\n",symbol,model->counts[symbol]);*/
	for(i=symbol;i <= SYMBOLS; i++) {
		model->counts[i]++;
	}
}

static freq model_get_symbol(const struct ari_model* model,const freq cf)
{
	const size_t* counts = model->counts;
	size_t sym_lo = 0;
	size_t sym_hi = SYMBOLS;

	do{
		size_t middle = (sym_lo+sym_hi+1)/2;
		/* we need +1 there, because we want last symbol that has freq <= cf */
		if(counts[middle] > cf) {
			sym_hi = middle - 1;
		}
		else if(counts[middle] < cf) {
			sym_lo = middle + 1;
		}
		else {
			sym_lo = middle;
		}

	} while(sym_lo < sym_hi);
	if(counts[sym_lo] > cf)
		sym_lo--;
	return sym_lo;
}
#endif
