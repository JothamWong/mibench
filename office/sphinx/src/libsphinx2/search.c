/* ====================================================================
 * Copyright (c) 1994-2000 Carnegie Mellon University.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "Sphinx" and "Carnegie Mellon" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. To obtain permission, contact 
 *    sphinx@cs.cmu.edu.
 *
 * 4. Products derived from this software may not be called "Sphinx"
 *    nor may "Sphinx" appear in their names without prior written
 *    permission of Carnegie Mellon University. To obtain permission,
 *    contact sphinx@cs.cmu.edu.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Carnegie
 *    Mellon University (http://www.speech.cs.cmu.edu/)."
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/*
 * search.c -- HMM-tree version
 * 
 * HISTORY
 * 
 * 30-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Generalized the implementation of pscr based allphone.
 * 		Added phone_conf option at the end of FWDTREE search to produce pscr-based
 * 		rescoring of fwdtree result.
 * 
 * 24-Mar-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added phone perplexity measure into search_hyp_t structure hypothesis
 * 		for each utterance.
 * 
 * 08-Mar-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lattice density measure into search_hyp_t structure generated
 * 		as hypothesis for each utterance.
 * 
 * 04-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added search_remove_context() call in search_postprocess_bptable.
 * 
 * 03-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Changed lm_cache_reset to lm3g_cache_reset, and lm_cache_stats_dump
 * 		to lm3g_cache_stats_dump.
 * 
 * 08-Dec-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Changed search_hyp_t hyp[] result to contain actual frame ids.
 * 		instead of post-silence-compression ids.
 * 		Added functions search_bptbl_wordlist() and search_bptbl_pred().
 * 
 * 12-Jul-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Commented out lm_cache_reset in search_fwdflat_start().
 * 
 * 19-Jun-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed strings phone_active to npa (phone_active is too generic).
 * 
 * 19-Jun-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added bestpscr[] and modified compute_phone_active() to use best phone
 * 		scores (bestpscr) returned by SCVQ.
 * 
 * 15-Jun-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified to always rebuild search tree when search_set_current_lm called.
 * 
 * 22-May-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *		Changed search_result and search_partial_result interfaces to simplify
 * 		network client interfaces for these two.
 * 
 * 09-Dec-94	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *		Added flat forward pass after tree forward pass.
 * 
 * Revision 8.10  94/10/11  12:39:52  rkm
 * Print back trace conditionally, depending on -backtrace argument.
 * 
 * Revision 8.9  94/07/29  12:01:40  rkm
 * Added code, in building search tree, to take care of growth in
 * dictionary size owing to dynamic addition of OOVs.
 * 
 * Revision 8.8  94/05/26  16:49:41  rkm
 * Rewrote lattice rescoring non-recursively to reduce LM thrashing,
 * and moved that code to a separate file.  Moved some data structure
 * declaration (BPTBL_T) to search.h.
 * 
 * Revision 8.7  94/05/19  14:21:36  rkm
 * Reordered LM accesses in last_phone_transition and word_transition for
 * greater efficiency.
 * 
 * Revision 8.6  94/05/10  10:48:43  rkm
 * Changed various list memory management routines to use the generic
 * listelem_alloc() and listelem_free() functions.
 * Added last_ltrans array for caching the best LM score info during
 * last_phone_transition().
 * 
 * Revision 8.5  94/04/22  13:56:30  rkm
 * Added search_hyp_t for collecting output hypothesis-related info in
 * one place.  Directed output of both passes to this structure, so the
 * match file will contain the final result.
 * 
 * Revision 8.4  94/04/14  14:43:52  rkm
 * Added optional second pass for lattice-rescoring.
 * Added option to dump forward pass bptable for postprocessing.
 * Added option to skip inter-channel transitions in alternate frames.
 * Fixed bug in init_search_tree in allocating first_phone_rchan_map.
 * 
 * Revision 8.1  94/02/15  15:13:07  rkm
 * Derived from v7.  Includes multiple start symbols for the LISTEN
 * project.  Includes multiple LMs for grammar switching.
 * 
 * Revision 1.14  94/02/11  13:12:48  rkm
 * Added multiple start words for the LISTEN project.
 * Corrected minor error in statistics gathering.
 * 
 * Revision 1.13  94/02/03  18:38:12  rkm
 * Fixed debugging and tracing code.
 * 
 * Revision 1.12  94/02/01  10:46:54  rkm
 * Mark active senones only if topN=4; otherwise SCVQ computes all senone scores.
 * 
 * Revision 1.11  94/02/01  10:23:02  rkm
 * Lookup trigram LM values through trigram LM cache instead of directly.
 * 
 * Revision 1.10  94/01/31  14:27:21  rkm
 * Added code to mark the active senones in each frame.
 * 
 * Revision 1.9  94/01/25  12:36:45  rkm
 * Look up LM values through bigram cache instead of directly.
 * 
 * Revision 1.8  94/01/24  10:01:38  rkm
 * Include LM score when entering last phone of any word, rather than on
 * exiting word.  Special case for handling single-phone words.
 * 
 * Revision 1.7  94/01/21  15:22:10  rkm
 * Minor changes.
 * 
 * Revision 1.6  94/01/21  15:06:45  rkm
 * Bug fix in word_transition in compacting BPTable.
 * 
 * Revision 1.5  94/01/21  13:47:48  rkm
 * Bug fix in alloc_all_rc().
 * 
 * Revision 1.4  94/01/19  11:25:48  rkm
 * Before rescoring last phone with LM scores.
 * 
 */
/*
 * NOTE: this module assumes that the dictionary is organized as follows:
 *     Main, real dictionary words
 *     </s>
 *     <s>... (possibly more than one of these)
 *     <sil>
 *     noise-words...
 * In particular, note that </s> comes before <s> since </s> occurs in the LM, but
 * not <s> (well, there's no transition to <s> in the LM).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "search_const.h"
#include "logmsg.h"
#include "err.h"
#include "dict.h"
#include "msd.h"
#include "lm.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "phone.h"
#include "kb.h"
#include "log.h"
#include "c.h"
#include "assert.h"
#include "scvq.h"
#include "fbs.h"
#include "search.h"
#include "hmm_tied_r.h"

#ifdef USE_ILM
#define lm_bg_score		ilm_bg_score
#define lm_tg_score		ilm_tg_score
#endif

#define ISA_FILLER_WORD(x)	((x) >= SilenceWordId)
#define ISA_REAL_WORD(x)	((x) < FinishWordId)

#define SEARCH_PROFILE			1
#define SEARCH_TRACE_CHAN		0
#define SEARCH_TRACE_CHAN_DETAILED	0
#define SEARCH_SELFTEST_DETAILED	0

/*
 * Search structure of HMM instances (channels; see CHAN_T and ROOT_CHAN_T definitions):
 * The word triphone sequences (HMM instances) are transformed into tree structures,
 * one tree per unique left triphone in the entire dictionary (actually diphone, since
 * its left context varies dyamically during the search process).  The entire set of
 * trees of channels is allocated once and for all during initialization (since
 * dynamic management of active CHANs is time consuming), with one exception: the
 * last phones of words, that need multiple right context modelling, are not maintained
 * in this static structure since there are too many of them and few are active at any
 * time.  Instead they are maintained as linked lists of CHANs, one list per word,
 * and each CHAN in this set is allocated only on demand and freed if inactive.
 */
static ROOT_CHAN_T *root_chan;	/* one per unique root channel */
static int32 n_root_chan_alloc;	/* total number of root channels allocated */
static int32 n_root_chan;	/* number of root channels valid for a given utt;
				   depends on words in the LM for that utt */
static int32 n_nonroot_chan;	/* #non-root channels in search tree */

/* MAX #non-root channels in search tree for allocating active_chan_list[]... */
static int32 max_nonroot_chan = 0;

static int32 n_phone_eval;
static int32 n_root_chan_eval;
static int32 n_nonroot_chan_eval;
static int32 n_last_chan_eval;
static int32 n_word_lastchan_eval;
static int32 n_lastphn_cand_utt;
static int32 n_phn_in_topsen;

/*
 * word_chan[w] = separate linked list of channels for each word w, normally used only
 * to model the last phone of w, with multiple channels representing different right
 * context phones.
 */
static CHAN_T **word_chan;

/* word_active[w] = 1 if word w active in current frame, 0 otherwise */
static int32 *word_active;

/*
 * Each node in the HMM tree structure may point to a set of words whose last phone
 * would follow that node in the tree structure (but is not included in the tree
 * structure for reasons explained above).  The channel node points to one word in this
 * set of words.  The remaining words are linked through homophone_set[].
 * 
 * Single-phone words are not represented in the HMM tree; they are kept in word_chan.
 *
 * Specifically, homophone_set[w] = wid of next word in the same set as w.
 */
static WORD_ID *homophone_set;

/*
 * In any frame, only some HMM tree nodes are active.  active_chan_list[f mod 2] =
 * list of nonroot channels in the HMM tree active in frame f.
 * Similarly, active_word_list[f mod 2] = list of word ids for which active channels
 * exist in word_chan in frame f.
 */
static CHAN_T **active_chan_list[2] = {NULL, NULL};
static int32 n_active_chan[2];		/* #entries in active_chan_list */
static WORD_ID *active_word_list[2];
static int32 n_active_word[2];		/* #entries in active_word_list */

static int32 NumWords;			/* Total #words in dictionary */
static int32 NumMainDictWords;		/* #words in main dictionary, excluding fillers
					   (i.e., <s>, </s>, <sil>, and noise words).
					   These come first in WordDict. */
static int32 NumHmmModels;
static int32 NumCiPhones;
static int32 TotalDists;

static SMD *Models;
static char **PhoneList;
static dictT *WordDict;
static LM LangModel = NULL;
static int32 UsingDarpaLM;
static int32 NoLangModel;
static int32 AllWordTProb;
/* static int32 AllWordMode; */

static int32 StartWordId;
static int32 FinishWordId;
static int32 SilenceWordId;
static int32 SilencePhoneId;

static int32 **LeftContextFwd;
static int32 **RightContextFwd;
static int32 **RightContextFwdPerm;
static int32 *RightContextFwdSize;
static int32 **LeftContextBwd;
static int32 **LeftContextBwdPerm;
static int32 *LeftContextBwdSize;
static int32 **RightContextBwd;

static int32 *distScores;		/* SC scores for current frame being searched */
static int32 **sc_scores;		/* SC scores for several frames in advance */

static int32 BestScore;			/* Best among all phones */
static int32 LastPhoneBestScore;	/* Best among last phones only */
static int32 LogBeamWidth;
static int32 NewPhoneLogBeamWidth;
static int32 NewWordLogBeamWidth;
static int32 LastPhoneAloneLogBeamWidth;
static int32 LastPhoneLogBeamWidth;

static int32 FwdflatLogBeamWidth;
static int32 FwdflatLogWordBeamWidth;

static int32 FillerWordPenalty = 0;
static int32 SilenceWordPenalty = 0;
static int32 LogInsertionPenalty = 0;
static int32 logPhoneInsertionPenalty = 0;
static double fwdtree_lw = 6.5;
static double fwdflat_lw = 8.5;
static double bestpath_lw = 9.5;

static int32 newword_penalty = 0;

/* BestScoreTable[CurrentFrame] === BestScore */
static int32 BestScoreTable[MAX_FRAMES];

static int32 ForcedRecMode;
static int32 compute_all_senones = TRUE;

static int32 ChannelsPerFrameTarget = 0;	/* #channels to eval / frame */
static int32 scVqTopN = 4;

static int32 CurrentFrame;
static int32 LastFrame;

extern int32 use_3g_in_fwd_pass;

int32 *senone_active;		/* list of active senones in current frame */
int32 n_senone_active;
char *senone_active_flag;
static int32 n_senone_active_utt;

static BPTBL_T *BPTable;	/* Forward pass lattice */
static int32 BPIdx;		/* First free BPTable entry */
static int32 BPTableSize;
static int32 *BScoreStack;	/* Score stack for all possible right contexts */
static int32 BSSHead;		/* First free BScoreStack entry */
static int32 BScoreStackSize;
static int32 *BPTableIdx;	/* First BPTable entry for each frame */
static int32 *WordLatIdx;	/* BPTable index for any word in current frame;
				   cleared before each frame */
static int32 BPTblOflMsg;	/* Whether BPtable overflow msg has been printed */

static int32 *lattice_density;	/* #words/frame in lattice */
static double *phone_perplexity;/* How sharply phones are discriminated/frame */

static int32 *zeroPermTab;

/* Word-id sequence hypothesized by decoder */
static search_hyp_t hyp[HYP_SZ];	/* no <s>, </s>, or filler words */
static char hyp_str[4096];		/* hyp as string of words sep. by blanks */
static int32 hyp_wid[4096];
static int32 n_hyp_wid;

static int32 HypTotalScore;
static int32 TotalLangScore;

static int32 hyp_alternates = FALSE;

int32 ForceIds[256];
int32 ForceLen = 0;

static WORD_ID *single_phone_wid;	/* list of single-phone word ids */
static int32 n_1ph_words;		/* #single phone words in dict (total) */
static int32 n_1ph_LMwords;		/* #single phone dict words also in LM;
					   these come first in single_phone_wid */

static void seg_back_trace (int32);
static void renormalize_scores (int32);
static void fwdflat_renormalize_scores (int32);
static int32 renormalized;

static int32 skip_alt_frm = 0;

/*
 * Two word context prior to utterance, to be used instead of <s>.  Faked by entering
 * <s> and the context into BPTable before starting search (see search_start_fwd).
 * If no context, both should be -ve; if only one context word, [1] should be -ve.
 */
static int32 context_word[2];

#if 0
static dump_search_tree_root ();
static dump_search_tree ();
#endif

/*
 * Declarations for ci-phone prediction based on top senones (applied only if
 * topsen_window > 1).  In each frame, lookahead topsen_window frames and pick the best
 * senones (i.e. senones within topsen_thresh of best senone in respective frames).
 * It is the potential set of future active phones.  So restrict phone transitions in
 * the current frame to this set.  (But filler phones are always active.)
 */
static int32 topsen_window = 1;		/* Lookahead window of frames over which top
					   senones used to predict ciphones.
					   No prediction if topsen_window == 1 */
static int32 n_topsen_frm;		/* #frames evaluated so far (lookahead version
					   of CurrentFrame) */
static int32 topsen_thresh = -60000;	/* Threshold for determining active phones from
					   top senone (initial value is a HACK!!) */
static int32 **npa_frm;			/* per frame next-phone-active[ciphone] flag, as
					   determined by top senones */
static int32 *npa;			/* npa_frm summed over topsen_window;
					   next-phone-active flag for activating phone
					   transitions. */
static int32 *filler_phone;		/* filler_phone[p] = 1 iff p is filler */

static int32 *topsen_score;		/* Top senone score in each frame */
static int32 *bestpscr;			/* Best senone score within each phone in frame */
static uint16 **utt_pscr = NULL;	/* bestpscr for entire utt; scaled */

static void topsen_init ( void );
static void compute_phone_active (int32 topsenscr, int32 npa_th);

/* FIXME: put this in a header file */
extern void quit (int status, char const *fmt, ...);

int32 context_frames ( void )
{
    if (context_word[0] < 0)
	return 0;
    if (context_word[1] < 0)
	return 2;
    return 3;
}

/*
 * Candidates words for entering their last phones.  Cleared and rebuilt in each
 * frame.
 * NOTE: candidates can only be multi-phone, real dictionary words.
 */
typedef struct lastphn_cand_s {
    int32 wid;
    int32 score;
    int32 bp;
    int32 next;		/* next candidate starting at the same frame */
} lastphn_cand_t;
static lastphn_cand_t *lastphn_cand;
static int32 n_lastphn_cand;

extern int32 print_back_trace;


#if 0
/*
 * Evaluate arcprobs of all active HMMs (actually sseqids) in current frame.
 */
void
evaluateModels (int32 fwd) 	/* True for the forward direction */
{
    long i, j, k;
    int32 *ap;
    int32 *tp;
    int32 *dist, tmp, *apl;
    
    for (j = n_phone_active, apl = phone_active; j > 0; --j, apl++) {
	i = *apl;
	
	ap = Models[i].arcProb;
	tp = Models[i].tp;
	dist = Models[i].dist;
	
	tmp = distScores[dist[0]];
	ap[0]  = tp[0]  + tmp;
	ap[1]  = tp[1]  + tmp;
	ap[2]  = tp[2]  + tmp;
	tmp = distScores[dist[3]];
	ap[3]  = tp[3]  + tmp;
	ap[4]  = tp[4]  + tmp;
	ap[5]  = tp[5]  + tmp;
	tmp = distScores[dist[6]];
	ap[6]  = tp[6]  + tmp;
	ap[7]  = tp[7]  + tmp;
	ap[8]  = tp[8]  + tmp;
	tmp = distScores[dist[9]];
	ap[9]  = tp[9]  + tmp;
	ap[10] = tp[10] + tmp;
	ap[11] = tp[11] + tmp;
	tmp = distScores[dist[12]];
	ap[12] = tp[12] + tmp;
	ap[13] = tp[13] + tmp;
    }
    k = n_phone_active;
    
#if SEARCH_PROFILE
    n_phone_eval += k;
#endif
    
#if SEARCH_TRACE_CHAN
    log_info ("[%4d] %8d models evaluated\n", CurrentFrame, k);
#endif
}

/*
 * Evaluate arc-probs of a single hmm (actually sseqid).
 */
void
eval_hmm_arcprob (int32 ssid)
{
    int32 *ap;
    int32 *tp;
    int32 *dist, tmp;
	
    ap = Models[ssid].arcProb;
    tp = Models[ssid].tp;
    dist = Models[ssid].dist;
    
    tmp = distScores[dist[0]];
    ap[0]  = tp[0]  + tmp;
    ap[1]  = tp[1]  + tmp;
    ap[2]  = tp[2]  + tmp;
    tmp = distScores[dist[3]];
    ap[3]  = tp[3]  + tmp;
    ap[4]  = tp[4]  + tmp;
    ap[5]  = tp[5]  + tmp;
    tmp = distScores[dist[6]];
    ap[6]  = tp[6]  + tmp;
    ap[7]  = tp[7]  + tmp;
    ap[8]  = tp[8]  + tmp;
    tmp = distScores[dist[9]];
    ap[9]  = tp[9]  + tmp;
    ap[10] = tp[10] + tmp;
    ap[11] = tp[11] + tmp;
    tmp = distScores[dist[12]];
    ap[12] = tp[12] + tmp;
    ap[13] = tp[13] + tmp;
}
#endif

/* Node Plus Arc (mpx channel) */
#define NPA(d,s,a)	(s + d->tp[a])


static void compute_phone_perplexity( void );
static search_hyp_t *fwdtree_pscr_path ( void );

void
root_chan_v_mpx_eval (ROOT_CHAN_T *chan)
{
    int32 bestScore;
    int32 s5, s4, s3, s2, s1, s0, t2, t1, t0;
    SMD *smd0, *smd1, *smd2, *smd3, *smd4;
    
    s4 = chan->score[4];
    smd4 = &Models[chan->sseqid[4]];
    s4 += distScores[smd4->dist[12]];
    
    s3 = chan->score[3];
    smd3 = &Models[chan->sseqid[3]];
    s3 += distScores[smd3->dist[9]];
    
    t1 = NPA(smd4,s4,13);
    t2 = NPA(smd3,s3,11);
    if (t1 > t2) {
	s5 = t1;
	chan->path[5]  = chan->path[4];
    } else {
	s5 = t2;
	chan->path[5]  = chan->path[3];
    }
    chan->score[5] = s5;
    bestScore = s5;
    
    s2 = chan->score[2];
    smd2 = &Models[chan->sseqid[2]];
    s2 += distScores[smd2->dist[6]];
    
    t0 = NPA(smd4,s4,12);
    t1 = NPA(smd3,s3,10);
    t2 = NPA(smd2,s2,8);
    if (t0 > t1) {
	if (t2 > t0) {
	    s4 = t2;
	    chan->path[4]  = chan->path[2];
	    chan->sseqid[4] = chan->sseqid[2];
	} else
	    s4 = t0;
    } else {
	if (t2 > t1) {
	    s4 = t2;
	    chan->path[4]  = chan->path[2];
	    chan->sseqid[4] = chan->sseqid[2];
	} else {
	    s4 = t1;
	    chan->path[4]  = chan->path[3];
	    chan->sseqid[4] = chan->sseqid[3];
	}
    }
    if (s4 > bestScore) bestScore = s4;
    chan->score[4] = s4;
    
    s1 = chan->score[1];
    smd1 = &Models[chan->sseqid[1]];
    s1 += distScores[smd1->dist[3]];
    
    t0 = NPA(smd3,s3,9);
    t1 = NPA(smd2,s2,7);
    t2 = NPA(smd1,s1,5);
    if (t0 > t1) {
	if (t2 > t0) {
	    s3 = t2;
	    chan->path[3]  = chan->path[1];
	    chan->sseqid[3] = chan->sseqid[1];
	} else
	    s3 = t0;
    } else {
	if (t2 > t1) {
	    s3 = t2;
	    chan->path[3]  = chan->path[1];
	    chan->sseqid[3] = chan->sseqid[1];
	} else {
	    s3 = t1;
	    chan->path[3]  = chan->path[2];
	    chan->sseqid[3] = chan->sseqid[2];
	}
    }
    if (s3 > bestScore) bestScore = s3;
    chan->score[3] = s3;
    
    s0 = chan->score[0];
    smd0 = &Models[chan->sseqid[0]];
    s0 += distScores[smd0->dist[0]];
    
    t0 = NPA(smd2,s2,6);
    t1 = NPA(smd1,s1,4);
    t2 = NPA(smd0,s0,2);
    if (t0 > t1) {
	if (t2 > t0) {
	    s2 = t2;
	    chan->path[2]  = chan->path[0];
	    chan->sseqid[2] = chan->sseqid[0];
	} else
	    s2 = t0;
    } else {
	if (t2 > t1) {
	    s2 = t2;
	    chan->path[2]  = chan->path[0];
	    chan->sseqid[2] = chan->sseqid[0];
	} else {
	    s2 = t1;
	    chan->path[2]  = chan->path[1];
	    chan->sseqid[2] = chan->sseqid[1];
	}
    }
    if (s2 > bestScore) bestScore = s2;
    chan->score[2] = s2;
    
    t0 = NPA(smd1,s1,3);
    t1 = NPA(smd0,s0,1);
    if (t0 > t1) {
	s1 = t0;
    } else {
	s1 = t1;
	chan->path[1]  = chan->path[0];
	chan->sseqid[1] = chan->sseqid[0];
    }
    if (s1 > bestScore) bestScore = s1;
    chan->score[1] = s1;
    
    s0 = NPA(smd0,s0,0);
    if (s0 > bestScore) bestScore = s0;
    chan->score[0] = s0;
    
    chan->bestscore = bestScore;
}

#define CHAN_V_EVAL(chan,smd) {			\
    int32 bestScore;				\
    int32 s5, s4, s3, s2, s1, s0, t2, t1, t0;	\
						\
    s4 = chan->score[4];			\
    s4 += distScores[smd->dist[12]];		\
    s3 = chan->score[3];			\
    s3 += distScores[smd->dist[9]];		\
    						\
    t1 = NPA(smd,s4,13);			\
    t2 = NPA(smd,s3,11);			\
    if (t1 > t2) {				\
	s5 = t1;				\
	chan->path[5]  = chan->path[4];		\
    } else {					\
	s5 = t2;				\
	chan->path[5]  = chan->path[3];		\
    }						\
    chan->score[5] = s5;			\
    bestScore = s5;				\
    						\
    s2 = chan->score[2];			\
    s2 += distScores[smd->dist[6]];		\
    						\
    t0 = NPA(smd,s4,12);			\
    t1 = NPA(smd,s3,10);			\
    t2 = NPA(smd,s2,8);				\
    if (t0 > t1) {				\
	if (t2 > t0) {				\
	    s4 = t2;				\
	    chan->path[4]  = chan->path[2];	\
	} else					\
	    s4 = t0;				\
    } else {					\
	if (t2 > t1) {				\
	    s4 = t2;				\
	    chan->path[4]  = chan->path[2];	\
	} else {				\
	    s4 = t1;				\
	    chan->path[4]  = chan->path[3];	\
	}					\
    }						\
    if (s4 > bestScore) bestScore = s4;		\
    chan->score[4] = s4;			\
    						\
    s1 = chan->score[1];			\
    s1 += distScores[smd->dist[3]];		\
    						\
    t0 = NPA(smd,s3,9);				\
    t1 = NPA(smd,s2,7);				\
    t2 = NPA(smd,s1,5);				\
    if (t0 > t1) {				\
	if (t2 > t0) {				\
	    s3 = t2;				\
	    chan->path[3]  = chan->path[1];	\
	} else					\
	    s3 = t0;				\
    } else {					\
	if (t2 > t1) {				\
	    s3 = t2;				\
	    chan->path[3]  = chan->path[1];	\
	} else {				\
	    s3 = t1;				\
	    chan->path[3]  = chan->path[2];	\
	}					\
    }						\
    if (s3 > bestScore) bestScore = s3;		\
    chan->score[3] = s3;			\
    						\
    s0 = chan->score[0];			\
    s0 += distScores[smd->dist[0]];		\
    						\
    t0 = NPA(smd,s2,6);				\
    t1 = NPA(smd,s1,4);				\
    t2 = NPA(smd,s0,2);				\
    if (t0 > t1) {				\
	if (t2 > t0) {				\
	    s2 = t2;				\
	    chan->path[2]  = chan->path[0];	\
	} else					\
	    s2 = t0;				\
    } else {					\
	if (t2 > t1) {				\
	    s2 = t2;				\
	    chan->path[2]  = chan->path[0];	\
	} else {				\
	    s2 = t1;				\
	    chan->path[2]  = chan->path[1];	\
	}					\
    }						\
    if (s2 > bestScore) bestScore = s2;		\
    chan->score[2] = s2;			\
    						\
    t0 = NPA(smd,s1,3);				\
    t1 = NPA(smd,s0,1);				\
    if (t0 > t1) {				\
	s1 = t0;				\
    } else {					\
	s1 = t1;				\
	chan->path[1]  = chan->path[0];		\
    }						\
    if (s1 > bestScore) bestScore = s1;		\
    chan->score[1] = s1;			\
    						\
    s0 = NPA(smd,s0,0);				\
    if (s0 > bestScore) bestScore = s0;		\
    chan->score[0] = s0;			\
    						\
    chan->bestscore = bestScore;		\
}						\


void
root_chan_v_eval (ROOT_CHAN_T *chan)
{
    SMD *smd0;
    
    smd0 = &(Models[chan->sseqid[0]]);
    CHAN_V_EVAL(chan,smd0);
}

void
chan_v_eval (CHAN_T *chan)
{
    SMD *smd0;
    
    smd0 = &(Models[chan->sseqid]);
    CHAN_V_EVAL(chan,smd0);
}

int32 eval_root_chan (void)
{
    ROOT_CHAN_T *rhmm;
    int32 i, cf, bestscore, k;
    
    cf = CurrentFrame;
    bestscore = WORST_SCORE;
    k = 0;
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	if (rhmm->active == cf) {
	    if (rhmm->mpx) {
		root_chan_v_mpx_eval (rhmm);
	    } else {
		root_chan_v_eval (rhmm);
	    }
	    
	    if (bestscore < rhmm->bestscore)
		bestscore = rhmm->bestscore;

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
	    k++;
#endif
	}
    }

#if SEARCH_PROFILE
    n_root_chan_eval += k;
#endif

#if SEARCH_TRACE_CHAN
    log_info (" %3d #root(%10d)", cf, k, bestscore);
#endif

    return (bestscore);
}

int32 eval_nonroot_chan (void)
{
    CHAN_T *hmm, **acl;
    int32 i, cf, bestscore, k;

    cf = CurrentFrame;
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    bestscore = WORST_SCORE;
    
    k = i;
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
	
	assert(hmm->active==cf);
	
	chan_v_eval (hmm);
	if (bestscore < hmm->bestscore)
	    bestscore = hmm->bestscore;
    }

#if SEARCH_PROFILE
    n_nonroot_chan_eval += k;
#endif

#if SEARCH_TRACE_CHAN
    log_info (" %5d #non-root(%10d)", k, bestscore);
#endif

    return (bestscore);
}

int32 eval_word_chan (void)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    int32 i, w, cf, bestscore, *awl, j, k;
    
    k = 0;
    cf = CurrentFrame;
    bestscore = WORST_SCORE;
    awl = active_word_list[cf & 0x1];
    
    i = n_active_word[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	assert(word_active[w] != 0);

	word_active[w] = 0;

	assert(word_chan[w] != NULL);

	for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
	    assert(hmm->active == cf);

	    chan_v_eval (hmm);

	    if (bestscore < hmm->bestscore)
		bestscore = hmm->bestscore;

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
	    k++;
#endif
	}
    }
    
    /* Similarly for statically allocated single-phone words */
    j = 0;
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active < cf)
	    continue;
	
	if (rhmm->mpx) {
	    root_chan_v_mpx_eval (rhmm);
	} else {
	    root_chan_v_eval (rhmm);
	}
	
	if ((bestscore < rhmm->bestscore) && (w != FinishWordId))
	    bestscore = rhmm->bestscore;
	
#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
	j++;
#endif
    }

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
    k += j;
#endif

#if SEARCH_PROFILE
    n_last_chan_eval += k;
    n_nonroot_chan_eval += k;
    n_word_lastchan_eval += n_active_word[cf & 0x1] + j;
#endif

#if SEARCH_TRACE_CHAN
    printf (" %5d #leaf(%10d)\n", k, bestscore);
#endif

    return (bestscore);
}

static void
cache_bptable_paths (int32 bp)
{
    int32 w, prev_bp;
    BPTBL_T *bpe;
    
    bpe = &(BPTable[bp]);
    prev_bp = bp;
    w = bpe->wid;
    while (ISA_FILLER_WORD(w)) {
	prev_bp = BPTable[prev_bp].bp;
	w = BPTable[prev_bp].wid;
    }
    bpe->real_fwid = WordDict->dict_list[w]->fwid;
    
    if (use_3g_in_fwd_pass) {
	prev_bp = BPTable[prev_bp].bp;
	bpe->prev_real_fwid = (prev_bp != NO_BP) ? BPTable[prev_bp].real_fwid : -1;
    } else
	bpe->prev_real_fwid = -1;
}

void
save_bwd_ptr (WORD_ID w, int32 score, int32 path, int32 rc)
{
    int32 _bp_;
    
    _bp_ = WordLatIdx[w];
    if (_bp_ != NO_BP) {
	if (BPTable[_bp_].score < score) {
	    if (BPTable[_bp_].bp != path) {
		BPTable[_bp_].bp = path;
		cache_bptable_paths (_bp_);
	    }
	    BPTable[_bp_].score = score;
	}
	BScoreStack[BPTable[_bp_].s_idx + rc] = score;
    } else {
	int32 i, rcsize, *bss;
	dict_entry_t *de;
	BPTBL_T *bpe;
	
	if ((BPIdx >= BPTableSize) || (BSSHead >= BScoreStackSize-NumCiPhones)) {
	    if (! BPTblOflMsg) {
		log_warn("%s(%d): BPTable OVERFLOWED; IGNORING REST OF UTTERANCE!!\n",
		       __FILE__, __LINE__);
		BPTblOflMsg = 1;
	    }
	    return;
	}
	
	de = WordDict->dict_list[w];
	WordLatIdx[w] = BPIdx;
	bpe = &(BPTable[BPIdx]);
	bpe->wid = w;
	bpe->frame = CurrentFrame;
	bpe->bp = path;
	bpe->score = score;
	bpe->s_idx = BSSHead;
	if ((de->len != 1) && (de->mpx)) {
	    bpe->r_diph = de->phone_ids[de->len - 1];
	    rcsize = RightContextFwdSize[bpe->r_diph];
	} else {
	    bpe->r_diph = -1;
	    rcsize = 1;
	}
	for (i = rcsize, bss = BScoreStack+BSSHead; i > 0; --i, bss++)
	    *bss = WORST_SCORE;
	BScoreStack[BSSHead + rc] = score;
	cache_bptable_paths (BPIdx);
	
	BPIdx++;
	BSSHead += rcsize;
    }
}

/*
 * Prune currently active root channels for next frame.  Also, perform exit
 * transitions out of them and activate successors.
 * score[] of pruned root chan set to WORST_SCORE elsewhere.
 */
void
prune_root_chan (void)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    int32 i, cf, nf, w, pip;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    CHAN_T **nacl;	/* next active list */
    lastphn_cand_t *candp;
    dict_entry_t *de;
    
    cf = CurrentFrame;
    nf = cf+1;
    thresh          = BestScore + LogBeamWidth;
    newphone_thresh = BestScore + NewPhoneLogBeamWidth;
    lastphn_thresh  = BestScore + LastPhoneLogBeamWidth;
    
    pip = logPhoneInsertionPenalty;
    
#if 0
    printf ("%10d =Th, %10d =NPTh, %10d =LPTh, %10d =bestscore\n",
	    thresh, newphone_thresh, lastphn_thresh, BestScore);
#endif

    nacl = active_chan_list[nf & 0x1];
    
    for (i = 0, rhmm = root_chan; i < n_root_chan; i++, rhmm++) {
	/* First check if this channel was active in current frame */
	if (rhmm->active < cf)
	    continue;
	
	if (rhmm->bestscore > thresh) {
	    rhmm->active = nf;		/* rhmm will be active in next frame */
	    
	    if (skip_alt_frm && (! (cf % skip_alt_frm)))
		continue;
	    
	    /* transitions out of this root channel */
	    newphone_score = rhmm->score[HMM_LAST_STATE] + pip;
	    if (newphone_score > newphone_thresh) {
		/* transition to all next-level channels in the HMM tree */
		for (hmm = rhmm->next; hmm; hmm = hmm->alt) {
		    if (npa[hmm->ciphone]) {
			if ((hmm->active < cf) || (hmm->score[0] < newphone_score)) {
			    hmm->score[0] = newphone_score;
			    hmm->path[0] = rhmm->path[HMM_LAST_STATE];
			    hmm->active = nf;
			    *(nacl++) = hmm;
			}
		    }
		}
		
		/*
		 * Transition to last phone of all words for which this is the
		 * penultimate phone (the last phones may need multiple right contexts).
		 * Remember to remove the temporary newword_penalty.
		 */
		if (newphone_score > lastphn_thresh) {
		    for (w = rhmm->penult_phn_wid; w >= 0; w = homophone_set[w]) {
			de = WordDict->dict_list[w];
			if (npa[de->ci_phone_ids[de->len-1]]) {
			    candp = lastphn_cand + n_lastphn_cand;
			    n_lastphn_cand++;
			    candp->wid = w;
			    candp->score = newphone_score - newword_penalty;
			    candp->bp = rhmm->path[HMM_LAST_STATE];
			}
		    }
		}
	    }
	}
    }
    n_active_chan[nf & 0x1] = nacl - active_chan_list[nf & 0x1];
}

/*
 * Prune currently active nonroot channels in HMM tree for next frame.  Also, perform
 * exit transitions out of such channels and activate successors.
 */
void
prune_nonroot_chan (void)
{
    CHAN_T *hmm, *nexthmm;
    int32 cf, nf, w, i, pip;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    CHAN_T **acl, **nacl;	/* active list, next active list */
    lastphn_cand_t *candp;
    dict_entry_t *de;
    
    cf = CurrentFrame;
    nf = cf+1;
    thresh          = BestScore + LogBeamWidth;
    newphone_thresh = BestScore + NewPhoneLogBeamWidth;
    lastphn_thresh  = BestScore + LastPhoneLogBeamWidth;
    
    pip = logPhoneInsertionPenalty;
    
    acl =  active_chan_list[cf & 0x1];	/* currently active HMMs in tree */
    nacl = active_chan_list[nf & 0x1] + n_active_chan[nf & 0x1];
    
    for (i = n_active_chan[cf & 0x1], hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
	assert(hmm->active >= cf);
	
	if (hmm->bestscore > thresh) {
	    /* retain this channel in next frame */
	    if (hmm->active != nf) {
		hmm->active = nf;
		*(nacl++) = hmm;
	    }
	    
	    if (skip_alt_frm && (! (cf % skip_alt_frm)))
		continue;
	    
	    /* transitions out of this channel */
	    newphone_score = hmm->score[HMM_LAST_STATE] + pip;
	    if (newphone_score > newphone_thresh) {
		/* transition to all next-level channel in the HMM tree */
		for (nexthmm = hmm->next; nexthmm; nexthmm = nexthmm->alt) {
		    if (npa[nexthmm->ciphone]) {
			if ((nexthmm->active < cf)||(nexthmm->score[0] < newphone_score)) {
			    nexthmm->score[0] = newphone_score;
			    nexthmm->path[0] = hmm->path[HMM_LAST_STATE];
			    if (nexthmm->active != nf) {
				nexthmm->active = nf;
				*(nacl++) = nexthmm;
			    }
			}
		    }
		}
		
		/*
		 * Transition to last phone of all words for which this is the
		 * penultimate phone (the last phones may need multiple right contexts).
		 * Remember to remove the temporary newword_penalty.
		 */
		if (newphone_score > lastphn_thresh) {
		    for (w = hmm->info.penult_phn_wid; w >= 0; w = homophone_set[w]) {
			de = WordDict->dict_list[w];
			if (npa[de->ci_phone_ids[de->len-1]]) {
			    candp = lastphn_cand + n_lastphn_cand;
			    n_lastphn_cand++;
			    candp->wid = w;
			    candp->score = newphone_score - newword_penalty;
			    candp->bp = hmm->path[HMM_LAST_STATE];
			}
		    }
		}
	    }
	} else if (hmm->active != nf) {
	    /* hmm->active = -1; */
	    hmm->bestscore = WORST_SCORE;
	    hmm->score[0] = WORST_SCORE;
	    hmm->score[1] = WORST_SCORE;
	    hmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	    hmm->score[3] = WORST_SCORE;
	    hmm->score[4] = WORST_SCORE;
#endif
	}
    }
    n_active_chan[nf & 0x1] = nacl - active_chan_list[nf & 0x1];
}

/*
 * Since the same instance of a word (i.e., <word,start-frame>) reaches its last
 * phone several times, we can compute its best BP and LM transition score info
 * just the first time and cache it for future occurrences.  Structure for such
 * a cache.
 */
typedef struct {
    int32 sf;		/* Start frame */
    int32 dscr;		/* Delta-score upon entering last phone */
    int32 bp;		/* Best BP */
} last_ltrans_t;
static last_ltrans_t *last_ltrans;	/* one per word */

#define CAND_SF_ALLOCSIZE	32
typedef struct {
    int32 bp_ef;
    int32 cand;
} cand_sf_t;
static int32 cand_sf_alloc = 0;
static cand_sf_t *cand_sf;

/*
 * Execute the transition into the last phone for all candidates words emerging from
 * the HMM tree.  Attach LM scores to such transitions.
 * (Executed after pruning root and non-root, but before pruning word-chan.)
 */
void
last_phone_transition (void)
{
    int32 i, j, k, cf, nf, bp, bplast, w;
    lastphn_cand_t *candp;
    int32 *nawl;
    int32 fwid2, thresh;
    int32 *rcpermtab, ciph0;
    int32 bestscore, dscr;
    dict_entry_t *de;
    CHAN_T *hmm;
    BPTBL_T *bpe;
    int32 n_cand_sf = 0;
    
    cf = CurrentFrame;
    nf = cf+1;
    nawl = active_word_list[nf & 0x1];
    
#if SEARCH_PROFILE
    n_lastphn_cand_utt += n_lastphn_cand;
#endif

    /* If best LM score and bp for candidate known use it, else sort cands by startfrm */
    for (i = 0, candp = lastphn_cand; i < n_lastphn_cand; i++, candp++) {
	bpe = &(BPTable[candp->bp]);
	rcpermtab = (bpe->r_diph >= 0) ? RightContextFwdPerm[bpe->r_diph] : zeroPermTab;
	
	/* Subtract starting score for candidate, leave it with only word score */
	de = WordDict->dict_list[candp->wid];
	ciph0 = de->ci_phone_ids[0];
	candp->score -= BScoreStack[bpe->s_idx + rcpermtab[ciph0]];
	
	/*
	 * If this candidate not occurred in an earlier frame, prepare for finding
	 * best transition score into last phone; sort by start frame.
	 */
	if (last_ltrans[candp->wid].sf != bpe->frame+1) {
	    for (j = 0; j < n_cand_sf; j++) {
		if (cand_sf[j].bp_ef == bpe->frame)
		    break;
	    }
	    if (j < n_cand_sf)
		candp->next = cand_sf[j].cand;
	    else {
		if (n_cand_sf >= cand_sf_alloc) {
		    if (cand_sf_alloc == 0) {
			cand_sf = (cand_sf_t *) CM_calloc (CAND_SF_ALLOCSIZE,
							   sizeof(cand_sf_t));
			cand_sf_alloc = CAND_SF_ALLOCSIZE;
		    } else {
			cand_sf_alloc += CAND_SF_ALLOCSIZE;
			cand_sf = (cand_sf_t *) CM_recalloc (cand_sf,
							     cand_sf_alloc,
							     sizeof(cand_sf_t));
			log_info("%s(%d): cand_sf[] increased to %d entries\n",
			      __FILE__, __LINE__, cand_sf_alloc);
		    }
		}
		
		j = n_cand_sf++;
		candp->next = -1;
		cand_sf[j].bp_ef = bpe->frame;
	    }
	    cand_sf[j].cand = i;
	    
	    last_ltrans[candp->wid].dscr = WORST_SCORE;
	    last_ltrans[candp->wid].sf = bpe->frame+1;
	}
    }
    
    /* Compute best LM score and bp for new cands entered in the sorted lists above */
    for (i = 0; i < n_cand_sf; i++) {
	/* For the i-th unique end frame... */
	bp = BPTableIdx[cand_sf[i].bp_ef];
	bplast = BPTableIdx[cand_sf[i].bp_ef+1]-1;
	
	for (bpe = &(BPTable[bp]); bp <= bplast; bp++, bpe++) {
	    /* For each bp entry in the i-th end frame... */
	    rcpermtab = (bpe->r_diph >= 0) ?
		RightContextFwdPerm[bpe->r_diph] : zeroPermTab;
	    
	    /* For each candidate at the start frame find bp->cand transition-score */
	    for (j = cand_sf[i].cand; j >= 0; j = candp->next) {
		candp = &(lastphn_cand[j]);
		de = WordDict->dict_list[candp->wid];
		ciph0 = de->ci_phone_ids[0];
		fwid2 = de->fwid;
		
		dscr = BScoreStack[bpe->s_idx + rcpermtab[ciph0]];
		dscr += lm_tg_score(bpe->prev_real_fwid, bpe->real_fwid, fwid2);
		
		if (last_ltrans[candp->wid].dscr < dscr) {
		    last_ltrans[candp->wid].dscr = dscr;
		    last_ltrans[candp->wid].bp = bp;
		}
	    }
	}
    }

    /* Update best transitions for all candidates; also update best lastphone score */
    bestscore = LastPhoneBestScore;
    for (i = 0, candp = lastphn_cand; i < n_lastphn_cand; i++, candp++) {
	candp->score += last_ltrans[candp->wid].dscr;
	candp->bp = last_ltrans[candp->wid].bp;
	
	if (bestscore < candp->score)
	    bestscore = candp->score;
    }
    LastPhoneBestScore = bestscore;
    
    /* At this pt, we know the best entry score (with LM component) for all candidates */
    thresh = bestscore + LastPhoneAloneLogBeamWidth;
    for (i = n_lastphn_cand, candp = lastphn_cand; i > 0; --i, candp++) {
	if (candp->score > thresh) {
	    w = candp->wid;

	    alloc_all_rc (w);
	    
	    k = 0;
	    for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
		if ((hmm->active < cf) || (hmm->score[0] < candp->score)) {
		    hmm->score[0] = candp->score;
		    hmm->path[0] = candp->bp;
		    
		    assert(hmm->active != nf);
		    
		    hmm->active = nf;
		    k++;
		}
	    }
	    if (k > 0) {
		assert(! word_active[w]);
		
		*(nawl++) = w;
		word_active[w] = 1;
	    }
	}
    }
    n_active_word[nf & 0x1] = nawl - active_word_list[nf & 0x1];
}

/*
 * Prune currently active word channels for next frame.  Also, perform exit
 * transitions out of such channels and active successors.
 */
void
prune_word_chan (void)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, *thmm;
    CHAN_T**phmmp;	/* previous HMM-pointer */
    int32 cf, nf, w, i, k;
    int32 newword_thresh, lastphn_thresh;
    int32 *awl, *nawl;
    
    cf = CurrentFrame;
    nf = cf+1;
    newword_thresh = LastPhoneBestScore + NewWordLogBeamWidth;
    lastphn_thresh = LastPhoneBestScore + LastPhoneAloneLogBeamWidth;

    awl =  active_word_list[cf & 0x1];
    nawl = active_word_list[nf & 0x1] + n_active_word[nf & 0x1];
    
    /* Dynamically allocated last channels of multi-phone words */
    for (i = n_active_word[cf & 0x1], w = *(awl++); i > 0; --i, w = *(awl++)) {
	k = 0;
	phmmp = &(word_chan[w]);
	for (hmm = word_chan[w]; hmm; hmm = thmm) {
	    assert(hmm->active >= cf);
	    
	    thmm = hmm->next;
	    if (hmm->bestscore > lastphn_thresh) {
		/* retain this channel in next frame */
		hmm->active = nf;
		k++;
		phmmp = &(hmm->next);
		
		/* Could if ((! skip_alt_frm) || (cf & 0x1)) the following */
		if (hmm->score[HMM_LAST_STATE] > newword_thresh) {
		    /* can exit channel and recognize word */
		    save_bwd_ptr (w, hmm->score[HMM_LAST_STATE],
				  hmm->path[HMM_LAST_STATE], hmm->info.rc_id);
		}
	    } else if (hmm->active == nf) {
		phmmp = &(hmm->next);
	    } else {
		listelem_free (hmm, sizeof(CHAN_T));
		*phmmp = thmm;
	    }
	}
	if ((k > 0) && (! word_active[w])) {
	    *(nawl++) = w;
	    word_active[w] = 1;
	}
    }
    n_active_word[nf & 0x1] = nawl - active_word_list[nf & 0x1];

    /*
     * Prune permanently allocated single-phone channels.
     * NOTES: score[] of pruned channels set to WORST_SCORE elsewhere.
     */
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active < cf)
	    continue;
	if (rhmm->bestscore > lastphn_thresh) {
	    rhmm->active = nf;

	    /* Could if ((! skip_alt_frm) || (cf & 0x1)) the following */
	    if (rhmm->score[HMM_LAST_STATE] > newword_thresh) {
		save_bwd_ptr (w, rhmm->score[HMM_LAST_STATE],
			      rhmm->path[HMM_LAST_STATE], 0);
	    }
	}
    }
}

/*
 * Allocate last phone channels for all possible right contexts for word w.  (Some
 * may already exist.)
 * (NOTE: Assume that w uses context!!)
 */
void
alloc_all_rc (int32 w)
{
    dict_entry_t *de;
    CHAN_T *hmm, *thmm;
    int32 *sseq_rc;	/* list of sseqid for all possible right context for w */
    int32 i;
    
    de = WordDict->dict_list[w];

    assert(de->mpx);
    
    sseq_rc = RightContextFwd[de->phone_ids[de->len-1]];
    
    hmm = word_chan[w];
    if ((hmm == NULL) || (hmm->sseqid != *sseq_rc)) {
	hmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
	hmm->next = word_chan[w];
	word_chan[w] = hmm;

	hmm->info.rc_id = 0;
	hmm->bestscore = WORST_SCORE;
	hmm->score[0] = WORST_SCORE;
	hmm->score[1] = WORST_SCORE;
	hmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	hmm->score[3] = WORST_SCORE;
	hmm->score[4] = WORST_SCORE;
#endif
	hmm->active = -1;
	hmm->sseqid = *sseq_rc;
    }
    for (i = 1, sseq_rc++; *sseq_rc >= 0; sseq_rc++, i++) {
	if ((hmm->next == NULL) || (hmm->next->sseqid != *sseq_rc)) {
	    thmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
	    thmm->next = hmm->next;
	    hmm->next = thmm;
	    hmm = thmm;
	    
	    hmm->info.rc_id = i;
	    hmm->bestscore = WORST_SCORE;
	    hmm->score[0] = WORST_SCORE;
	    hmm->score[1] = WORST_SCORE;
	    hmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	    hmm->score[3] = WORST_SCORE;
	    hmm->score[4] = WORST_SCORE;
#endif
	    hmm->active = -1;
	    hmm->sseqid = *sseq_rc;
	} else
	    hmm = hmm->next;
    }
}

void
free_all_rc (int32 w)
{
    CHAN_T *hmm, *thmm;
    
    for (hmm = word_chan[w]; hmm; hmm = thmm) {
	thmm = hmm->next;
	listelem_free (hmm, sizeof(CHAN_T));
    }
    word_chan[w] = NULL;
}

/*
 * Structure for reorganizing the BP table entries in the current frame according
 * to distinct right context ci-phones.  Each entry contains the best BP entry for
 * a given right context.  Each successor word will pick up the correct entry based
 * on its first ci-phone.
 */
struct bestbp_rc_s {
    int32 score;
    int32 path;		/* BP table index corresponding to this entry */
    int32 lc;		/* right most ci-phone of above BP entry word */
} *bestbp_rc;

void
word_transition (void)
{
    int32 i, k, bp, w, cf, nf;
    /* int32 prev_bp, prev_wid, prev_endframe, prev2_bp, prev2_wid; */
    int32 /* rcsize, */ rc;
    int32 *rcss;		/* right context score stack */
    int32 *rcpermtab;
    int32 thresh, /* newword_thresh, */ newscore;
    BPTBL_T *bpe;
    dict_entry_t *pde, *de;	/* previous dict entry, dict entry */
    ROOT_CHAN_T *rhmm;
    /* CHAN_T *hmm; */
    struct bestbp_rc_s *bestbp_rc_ptr;
    int32 last_ciph;
    int32 /* fwid0, fwid1, */ fwid2;
    int32 pip;
    int32 ssid;
    
    cf = CurrentFrame;

    /*
     * Transition to start of new word instances (HMM tree roots); but only if words
     * other than </s> finished here.
     * But, first, find the best starting score for each possible right context phone.
     */
    for (i = NumCiPhones-1; i >= 0; --i)
	bestbp_rc[i].score = WORST_SCORE;
    k = 0;
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
	bpe = &(BPTable[bp]);
	WordLatIdx[bpe->wid] = NO_BP;
	
	if (bpe->wid == FinishWordId)
	    continue;
	k++;
	
	de = WordDict->dict_list[bpe->wid];
	rcpermtab = (bpe->r_diph >= 0) ? RightContextFwdPerm[bpe->r_diph] : zeroPermTab;
	last_ciph = de->ci_phone_ids[de->len-1];
	
	rcss = &(BScoreStack[bpe->s_idx]);
	for (rc = NumCiPhones-1; rc >= 0; --rc) {
	    if (rcss[rcpermtab[rc]] > bestbp_rc[rc].score) {
		bestbp_rc[rc].score = rcss[rcpermtab[rc]];
		bestbp_rc[rc].path = bp;
		bestbp_rc[rc].lc = last_ciph;
	    }
	}
    }
    if (k == 0)
	return;
    
    nf = cf+1;
    thresh = BestScore + LogBeamWidth;
    pip = logPhoneInsertionPenalty;
    /*
     * Hypothesize successors to words finished in this frame.
     * Main dictionary, multi-phone words transition to HMM-trees roots.
     */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	bestbp_rc_ptr = &(bestbp_rc[rhmm->ciphone]);

	if (npa[rhmm->ciphone]) {
	    newscore = bestbp_rc_ptr->score + newword_penalty + pip;
	    if (newscore > thresh) {
		if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
		    ssid = LeftContextFwd[rhmm->diphone][bestbp_rc_ptr->lc];
		    rhmm->score[0] = newscore;
		    rhmm->path[0] = bestbp_rc_ptr->path;
		    rhmm->active = nf;
		    rhmm->sseqid[0] = ssid;
		}
	    }
	}
    }
    
    /*
     * Single phone words; no right context for these.  Cannot use bestbp_rc as
     * LM scores have to be included.  First find best transition to these words.
     */
    for (i = 0; i < n_1ph_LMwords; i++) {
	w = single_phone_wid[i];
	last_ltrans[w].dscr = (int32) 0x80000000;
    }
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
	bpe = &(BPTable[bp]);
	rcpermtab = (bpe->r_diph >= 0) ? RightContextFwdPerm[bpe->r_diph] : zeroPermTab;
	rcss = BScoreStack + bpe->s_idx;
	
	for (i = 0; i < n_1ph_LMwords; i++) {
	    w = single_phone_wid[i];
	    de = WordDict->dict_list[w];
	    fwid2 = de->fwid;

	    newscore = rcss[rcpermtab[de->ci_phone_ids[0]]];
	    newscore += lm_tg_score(bpe->prev_real_fwid, bpe->real_fwid, fwid2);
	    
	    if (last_ltrans[w].dscr < newscore) {
		last_ltrans[w].dscr = newscore;
		last_ltrans[w].bp = bp;
	    }
	}
    }

    /* Now transition to in-LM single phone words */
    for (i = 0; i < n_1ph_LMwords; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if ((w != FinishWordId) && (! npa[rhmm->ciphone]))
	    continue;

	if ((newscore = last_ltrans[w].dscr + pip) > thresh) {
	    bpe = BPTable + last_ltrans[w].bp;
	    pde = WordDict->dict_list[bpe->wid];
	    
	    if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
		rhmm->score[0] = newscore;
		rhmm->path[0] = last_ltrans[w].bp;
		if (rhmm->mpx)
		    rhmm->sseqid[0] =
			LeftContextFwd[rhmm->diphone][pde->ci_phone_ids[pde->len-1]];
		rhmm->active = nf;
	    }
	}
    }
    
    /* Remaining words: <sil>, noise words.  No mpx for these! */
    bestbp_rc_ptr = &(bestbp_rc[SilencePhoneId]);
    newscore = bestbp_rc_ptr->score + SilenceWordPenalty + pip;
    if (newscore > thresh) {
	w = SilenceWordId;
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
	    rhmm->score[0] = newscore;
	    rhmm->path[0] = bestbp_rc_ptr->path;
	    rhmm->active = nf;
	}
    }
    newscore = bestbp_rc_ptr->score + FillerWordPenalty + pip;
    if (newscore > thresh) {
	for (w = SilenceWordId+1; w < NumWords; w++) {
	    rhmm = (ROOT_CHAN_T *) word_chan[w];
	    if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
		rhmm->score[0] = newscore;
		rhmm->path[0] = bestbp_rc_ptr->path;
		rhmm->active = nf;
	    }
	}
    }
}


#if 0
static void dump_hmm_tp ( void )
{
    int32 p, ssid, t;
    
    for (p = 0; p < NumCiPhones; p++) {
	ssid = hmm_pid2sid (p);
	printf ("sseqid(%s)= %d, ", phone_from_id (p), ssid);
	for (t = 0; t < 14; t++)
	    printf (" %6d", Models[ssid].tp[t]);
	printf ("\n");
    }
}
#endif


void
search_initialize (void)
{
    int32 bptable_size = query_lattice_size();
    
#if SEARCH_TRACE_CHAN_DETAILED
    static void load_trace_wordlist ();
#endif

    ForcedRecMode = FALSE;
    
    NumWords =            kb_get_num_words ();
    NumHmmModels =        kb_get_num_models ();
    TotalDists =          kb_get_total_dists ();
    Models =              kb_get_models ();
    PhoneList =           kb_get_phone_list ();
    WordDict =            kb_get_word_dict ();
    StartWordId =         kb_get_word_id (kb_get_lm_start_sym());
    FinishWordId =        kb_get_word_id (kb_get_lm_end_sym());
    SilenceWordId =       kb_get_word_id ("SIL");
    UsingDarpaLM =        kb_get_darpa_lm_flag ();
    AllWordTProb =        kb_get_aw_tprob();
    NoLangModel =         kb_get_no_lm_flag();
    SilencePhoneId =      phone_to_id ("SIL", TRUE);
    NumCiPhones =         phoneCiCount();
    
    LeftContextFwd =      dict_left_context_fwd ();
    RightContextFwd =     dict_right_context_fwd ();
    RightContextFwdPerm = dict_right_context_fwd_perm ();
    RightContextFwdSize = dict_right_context_fwd_size ();
    LeftContextBwd =      dict_left_context_bwd ();
    LeftContextBwdPerm =  dict_left_context_bwd_perm ();
    LeftContextBwdSize =  dict_left_context_bwd_size ();
    RightContextBwd =     dict_right_context_bwd ();
    NumMainDictWords =    dict_get_num_main_words (WordDict);
    
    word_chan = (CHAN_T **) CM_calloc (NumWords, sizeof (CHAN_T *));
    WordLatIdx =  (int32 *) CM_calloc (NumWords, sizeof(int32));
    zeroPermTab = (int32 *) CM_calloc (phoneCiCount(), sizeof(int32));
    word_active = (int32 *) CM_calloc (NumWords, sizeof(int32));
    
    BPTableSize = MAX (25, NumWords/1000) * MAX_FRAMES;
    BScoreStackSize = BPTableSize*20;
    if ((bptable_size > 0) && (bptable_size < 0x7fffffff)) {
	BPTableSize = bptable_size;
	BScoreStackSize = BPTableSize*20;	/* 20 = ave. rc fanout */
    }
    BPTable =   (BPTBL_T *) CM_calloc (BPTableSize, sizeof (BPTBL_T));
    BScoreStack = (int32 *) CM_calloc (BScoreStackSize, sizeof(int32));
    BPTableIdx =  (int32 *) CM_calloc (MAX_FRAMES+2, sizeof(int32));
    BPTableIdx++;	/* Make BPTableIdx[-1] valid */
    
    lattice_density = (int32 *) CM_calloc (MAX_FRAMES, sizeof(int32));
    phone_perplexity = (double *) CM_calloc (MAX_FRAMES, sizeof(double));
    
    init_search_tree (WordDict);
    
    active_word_list[0] = (WORD_ID *) CM_calloc (2*(NumWords+1), sizeof (WORD_ID));
    active_word_list[1] = active_word_list[0] + NumWords+1;

    bestbp_rc = (struct bestbp_rc_s *) CM_calloc (NumCiPhones,
						  sizeof (struct bestbp_rc_s));
#if SEARCH_TRACE_CHAN_DETAILED
    load_trace_wordlist ("_TRACEWORDS_");
#endif
    lastphn_cand = (lastphn_cand_t *) CM_calloc (NumWords, sizeof(lastphn_cand_t));
    
    senone_active = (int32 *) CM_calloc (TotalDists, sizeof(int32));
    senone_active_flag = (char *) CM_calloc (TotalDists, sizeof(char));

    last_ltrans = (last_ltrans_t *) CM_calloc (NumWords, sizeof(last_ltrans_t));

    search_fwdflat_init ();
    searchlat_init ();
    
    context_word[0] = -1;
    context_word[1] = -1;
    
    /*
     * Frames window size for predicting phones based on topsen.
     * If 1, no prediction; use all phones.
     */
    if ((topsen_window = query_topsen_window ()) < 1)
	quit(-1, "%s(%d): topsen window = %d\n", __FILE__, __LINE__, topsen_window);
    log_info ("%s(%d): topsen-window = %d", __FILE__, __LINE__, topsen_window);
    topsen_thresh = query_topsen_thresh ();
    if (topsen_window > 1)
	log_info (", threshold = %d", topsen_thresh);
    else
	log_info (", no phone-prediction");
    log_info ("\n");
    
    topsen_init ();

    sc_scores = (int32 **) CM_2dcalloc (topsen_window, TotalDists, sizeof(int32));
    distScores = sc_scores[0];

    topsen_score = (int32 *) CM_calloc (MAX_FRAMES, sizeof(int32));

    /* Inform SCVQ module of senones/phone and bestscore/phone array */
    {
	bestpscr = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
	utt_pscr = (uint16 **) CM_2dcalloc (MAX_FRAMES, NumCiPhones,
						    sizeof(uint16));
	scvq_set_psen (NumCiPhones, hmm_get_psen());
	scvq_set_bestpscr (bestpscr);
    }
}

int32 *search_get_dist_scores(void)
{
    return distScores;
}

#if 0
search_init (beam_width, all_word_mode, force_str)
    float beam_width;		/* Width of beam threshold */
    int32 all_word_mode;	/* True if we're to run in all_word_mode */
    char *force_str;		/* Non-NULL iff forced recognition mode */
{
    char *startWord;
    
    /* For LISTEN project */
    startWord = get_current_startword ();
    if (*startWord) {
        StartWordId = kb_get_word_id (startWord);
        log_info("startword %s -> %d (default is %d)\n",
                startWord,
                StartWordId,
                kb_get_word_id (kb_get_lm_start_sym()));
	if (StartWordId == -1) {
	        StartWordId = kb_get_word_id (kb_get_lm_start_sym());
		log_warn("Using default startwordid %d\n",
			StartWordId);
	}
    } else {
        StartWordId = kb_get_word_id (kb_get_lm_start_sym());
    }
    
    LogBeamWidth =  8 * LOG (beam_width);
    
    AllWordMode = all_word_mode;
    
    /*
     * If there is reference str we assume we are in forced rec mode
     */
    if (force_str) {
	quit(-1, "%s(%d): forced recognition not implemented\n", __FILE__, __LINE__);
	ForcedRecMode = TRUE;
	ForceLen = parse_ref_str (force_str, ForceIds);
    } else {
	ForceLen = 0;
	ForcedRecMode = FALSE;
    }
    
    return 0;
}
#endif


void search_set_startword (char const *str)
{
    char const *startWord;
    
    /* For LISTEN project */
    startWord = str;
    if (*startWord) {
        if ((StartWordId = kb_get_word_id (startWord)) < 0) {
	    startWord = kb_get_lm_start_sym();
	    StartWordId = kb_get_word_id (startWord);
	}
    } else {
	startWord = kb_get_lm_start_sym();
        StartWordId = kb_get_word_id (startWord);
    }
    log_info("%s(%d): startword= %s (id= %d)\n",
	     __FILE__, __LINE__, startWord, StartWordId);
}


/*
 * Set previous two LM context words for search; ie, the first word decoded by
 * search will use w1 and w2 as history for trigram scoring.  If w2 < 0, only
 * one-word history available.  If both < 0, no history available.
 */
void search_set_context (int32 w1, int32 w2)
{
    context_word[0] = w1;
    context_word[1] = w2;
}

/*
 * Hyp produced by search includes w1 and w2 from search_set_context() as the first
 * two words.  Remove them, if present, from hyp.
 */
void search_remove_context (search_hyp_t *hyp)
{
    int32 i, j;

    j = 0;
    if (context_word[0] >= 0)
	j++;
    if (context_word[1] >= 0)
	j++;
    if (j > 0) {
	for (i = j; hyp[i].wid >= 0; i++)
	    hyp[i-j] = hyp[i];
	hyp[i-j].wid = -1;

	/* hyp_wid has EVERYTHING, including the initial <s>; remove context after it */
	for (i = j+1; i < n_hyp_wid; i++)
	    hyp[i-j] = hyp[i];
	n_hyp_wid -= j;
    }
}

/*
 * Mark the active senones for all senones belonging to channels that are active in the
 * current frame.
 */
void
compute_sen_active (void)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, **acl;
    int32 i, j, cf, w, *awl, s, *dist, d;
    char *flagptr;
    
    cf = CurrentFrame;
    
    memset (senone_active_flag, 0, TotalDists * sizeof(char));
    n_senone_active = 0;
    
    /* Flag active senones for root channels */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	if (rhmm->active == cf) {
	    if (rhmm->mpx) {
		for (s = 0; s < HMM_LAST_STATE; s++) {
		    dist = Models[rhmm->sseqid[s]].dist;
		    d = dist[s*3];
		    senone_active_flag[d] = 1;
		}
	    } else {
		dist = Models[rhmm->sseqid[0]].dist;
		for (s = 0; s < TRANS_CNT; s += 3) {
		    d = dist[s];
		    senone_active_flag[d] = 1;
		}
	    }
	}
    }

    /* Flag active senones for nonroot channels in HMM tree */
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
	dist = Models[hmm->sseqid].dist;
	for (s = 0; s < TRANS_CNT; s += 3) {
	    d = dist[s];
	    senone_active_flag[d] = 1;
	}
    }
    
    /* Flag active senones for individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
	    dist = Models[hmm->sseqid].dist;
	    for (s = 0; s < TRANS_CNT; s += 3) {
		d = dist[s];
		senone_active_flag[d] = 1;
	    }
	}
    }
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    if (rhmm->mpx) {
		for (s = 0; s < HMM_LAST_STATE; s++) {
		    dist = Models[rhmm->sseqid[s]].dist;
		    d = dist[s*3];
		    senone_active_flag[d] = 1;
		}
	    } else {
		dist = Models[rhmm->sseqid[0]].dist;
		for (s = 0; s < TRANS_CNT; s += 3) {
		    d = dist[s];
		    senone_active_flag[d] = 1;
		}
	    }
	}
    }

    /* Form active senone list */
    j = 0;
    for (i = 0, flagptr = senone_active_flag; i < TotalDists; i++, flagptr++)
	if (*flagptr)
	    senone_active[j++] = i;
    n_senone_active = j;
}


/*
 * Tree-Search one frame forward.
 */
void
search_fwd (float *cep, float *dcep, float *dcep_80ms, float *pcep, float *ddcep)
{
    int32 *newscr;
    int32 i, cf;
    
    /* Rotate senone scores arrays by one frame, recycling the oldest one */
    distScores = sc_scores[0];
    for (i = 0; i < topsen_window-1; i++)
	sc_scores[i] = sc_scores[i+1];
    sc_scores[topsen_window-1] = distScores;
    newscr = distScores;
    
    /* Compute senone scores */
    cf = CurrentFrame;

    if (! compute_all_senones) {
	compute_sen_active ();
	topsen_score[cf] = SCVQScores(newscr, cep, dcep, dcep_80ms, pcep, ddcep);
    } else {
	topsen_score[cf] = SCVQScores_all(newscr, cep, dcep, dcep_80ms, pcep, ddcep);
    }
    n_senone_active_utt += n_senone_active;
    
    if (topsen_window > 1) {
	/*
	 * Predict next active phones (npa) from top senones within next topsen_window
	 * frames, including the current frame.
	 */
	compute_phone_active (topsen_score[cf], topsen_thresh);
	distScores = sc_scores[0];
	n_topsen_frm++;
    } else
	distScores = newscr;
    
    if ((topsen_window == 1) || (n_topsen_frm >= topsen_window))
	search_one_ply_fwd ();
}

void
search_start_fwd (void)
{
    int32 i, rcsize, lscr;
    ROOT_CHAN_T *rhmm;
    dict_entry_t *de;
    
    n_phone_eval = 0;
    n_root_chan_eval = 0;
    n_nonroot_chan_eval = 0;
    n_last_chan_eval = 0;
    n_word_lastchan_eval = 0;
    n_lastphn_cand_utt = 0;
    n_phn_in_topsen = 0;
    n_senone_active_utt = 0;
    
    BPIdx = 0;
    BSSHead = 0;
    BPTblOflMsg = 0;
    CurrentFrame = 0;
    
    for (i = 0; i < NumWords; i++)
	WordLatIdx[i] = NO_BP;
    
    lm3g_cache_reset ();

    n_active_chan[0] = 0;
    n_active_word[0] = 0;
    
    BestScore = 0;
    renormalized = 0;
    
    for (i = 0; i < NumWords; i++)
	last_ltrans[i].sf = -1;

    hyp_str[0] = '\0';
    hyp[0].wid = -1;
    
    if (context_word[0] < 0) {
	/* Start search with <s>; word_chan[<s>] is permanently allocated */
	rhmm = (ROOT_CHAN_T *) word_chan[StartWordId];
	rhmm->score[0] = 0;
	rhmm->score[1] = WORST_SCORE;
	rhmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	rhmm->score[3] = WORST_SCORE;
	rhmm->score[4] = WORST_SCORE;
#endif
	rhmm->bestscore = WORST_SCORE;
	rhmm->path[0] = NO_BP;
	rhmm->active = 0;		/* Frame in which active */
    } else {
	/* Simulate insertion of context words into BPTable; first <s> */
	BPTableIdx[0] = 0;
	save_bwd_ptr (StartWordId, 0, NO_BP, 0);
	WordLatIdx[StartWordId] = NO_BP;
	CurrentFrame++;
	
	/* Insert first context word */
	BPTableIdx[1] = 1;
	de = WordDict->dict_list[context_word[0]];
	rcsize = (de->mpx && (de->len > 1)) ?
	    RightContextFwdSize[de->phone_ids[de->len-1]] : 1;
	lscr = lm_bg_score (StartWordId, context_word[0]);
	for (i = 0; i < rcsize; i++)
	    save_bwd_ptr (context_word[0], lscr, 0, i);
	WordLatIdx[context_word[0]] = NO_BP;
	CurrentFrame++;
	
	/* Insert 2nd context word, if any */
	if (context_word[1] >= 0) {
	    BPTableIdx[2] = 2;
	    de = WordDict->dict_list[context_word[1]];
	    rcsize = (de->mpx && (de->len > 1)) ?
		RightContextFwdSize[de->phone_ids[de->len-1]] : 1;
	    lscr += lm_tg_score (StartWordId, context_word[0], context_word[1]);
	    for (i = 0; i < rcsize; i++)
		save_bwd_ptr (context_word[1], lscr, 1, i);
	    WordLatIdx[context_word[0]] = NO_BP;
	    CurrentFrame++;

	    n_active_chan[1] = 0;
	    n_active_word[1] = 0;
	}

	/* Search from silence */
	rhmm = (ROOT_CHAN_T *) word_chan[SilenceWordId];
	rhmm->score[0] = BPTable[BPIdx-1].score;
	rhmm->score[1] = WORST_SCORE;
	rhmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	rhmm->score[3] = WORST_SCORE;
	rhmm->score[4] = WORST_SCORE;
#endif
	rhmm->bestscore = WORST_SCORE;
	rhmm->path[0] = BPIdx-1;
	rhmm->active = CurrentFrame;		/* Frame in which active */
    }

    compute_all_senones = query_compute_all_senones() || (topsen_window > 1);

    if (topsen_window > 1) {
	/* Initialize next-phone-active flags */
	memset (npa, 0, NumCiPhones * sizeof(int32));
	for (i = 0; i < topsen_window; i++)
	    memset (npa_frm[i], 0, NumCiPhones * sizeof(int32));
    }
    n_topsen_frm = 0;
}

void
evaluateChannels (void)
{
    int32 bs;
    
    BestScore = eval_root_chan ();
    if ((bs = eval_nonroot_chan ()) > BestScore)
	BestScore = bs;
    if ((bs = eval_word_chan ()) > BestScore)
	BestScore = bs;
    LastPhoneBestScore = bs;
    BestScoreTable[CurrentFrame] = BestScore;
}

void
pruneChannels (void)
{
    n_lastphn_cand = 0;
    prune_root_chan ();
    prune_nonroot_chan ();
    last_phone_transition ();
    prune_word_chan ();
}

void
search_one_ply_fwd (void)
{
    int32 /* bs, */ i, cf, nf, w; /*, *awl; */
    ROOT_CHAN_T *rhmm;
    
    if (CurrentFrame >= MAX_FRAMES-1)
	return;
    
    BPTableIdx[CurrentFrame] = BPIdx;
    
    /* Need to renormalize? */
    if ((BestScore + (2 * LogBeamWidth)) < WORST_SCORE) {
	log_info("%s(%d): Renormalizing Scores at frame %d, best score %d\n",
		 __FILE__, __LINE__, CurrentFrame, BestScore);
	renormalize_scores (BestScore);
    }
    
    BestScore = WORST_SCORE;
    
#if SEARCH_TRACE_CHAN_DETAILED
    log_info ("[%4d] CHAN trace before eval\n", CurrentFrame);
    dump_traceword_chan ();
#endif
    
    evaluateChannels();
    
#if SEARCH_TRACE_CHAN_DETAILED
    log_info ("[%4d] CHAN trace after eval\n", CurrentFrame);
    dump_traceword_chan ();
#endif
    
    pruneChannels ();
    
    /* Do inter-word transitions */
    if (BPTableIdx[CurrentFrame] < BPIdx)
	word_transition ();

    /* Clear score[] of pruned root channels (UGLY!) */
    cf = CurrentFrame;
    nf = cf+1;
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	if (rhmm->active == cf) {
	    rhmm->bestscore = WORST_SCORE;
	    rhmm->score[0] = WORST_SCORE;
	    rhmm->score[1] = WORST_SCORE;
	    rhmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	    rhmm->score[3] = WORST_SCORE;
	    rhmm->score[4] = WORST_SCORE;
#endif
	}
    }
    /* Clear score[] of pruned single-phone channels (UGLY!) */
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    rhmm->bestscore = WORST_SCORE;
	    rhmm->score[0] = WORST_SCORE;
	    rhmm->score[1] = WORST_SCORE;
	    rhmm->score[2] = WORST_SCORE;
#if HMM_5_STATE
	    rhmm->score[3] = WORST_SCORE;
	    rhmm->score[4] = WORST_SCORE;
#endif
	}
    }
    
    /* This code terminates the loop by updating for the next pass */
    CurrentFrame++;
    if (CurrentFrame >= MAX_FRAMES-1) {
	log_warn ("%s(%d): MAX_FRAMES (%d) EXCEEDED; IGNORING REST OF UTTERANCE!!\n",
		__FILE__, __LINE__, MAX_FRAMES);
    }
    
    lm_next_frame ();
}

void
search_finish_fwd (void)
{
    /* register int32 idx; */
    int32 i, j, w, cf, nf, /* f, */ *awl;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, /* *thmm,*/ **acl;
    /* int32 bp, bestbp, bestscore; */
    /* int32 l_scr; */
    
    if ((CurrentFrame > 0) && (topsen_window > 1)) {
	/* Wind up remaining frames */
	for (i = 1; i < topsen_window; i++) {
	    distScores = sc_scores[i];
	    search_one_ply_fwd ();
	}
    }
    
    BPTableIdx[CurrentFrame] = BPIdx;
    if (CurrentFrame > 0)
	CurrentFrame--;
    LastFrame = CurrentFrame;
    
    /* Deactivate channels lined up for the next frame */
    cf = CurrentFrame;
    nf = cf+1;
    /* First, root channels of HMM tree */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	rhmm->active = -1;
	for (j = 0; j < HMM_LAST_STATE; j++)
	    rhmm->score[j] = WORST_SCORE;
	rhmm->bestscore = WORST_SCORE;
    }
	
    /* nonroot channels of HMM tree */
    i = n_active_chan[nf & 0x1];
    acl = active_chan_list[nf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
	hmm->active = -1;
	for (j = 0; j < HMM_LAST_STATE; j++)
	    hmm->score[j] = WORST_SCORE;
	hmm->bestscore = WORST_SCORE;
    }
    
    /* word channels */
    i = n_active_word[nf & 0x1];
    awl = active_word_list[nf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	word_active[w] = 0;
	free_all_rc (w);
    }
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	rhmm->active = -1;
	for (j = 0; j < HMM_LAST_STATE; j++)
	    rhmm->score[j] = WORST_SCORE;
	rhmm->bestscore = WORST_SCORE;
    }

    /* Obtain lattice density and phone perplexity info for this utterance */
    bptbl2latdensity(BPIdx, lattice_density);
    compute_phone_perplexity ();
    
    search_postprocess_bptable ((double)1.0, "FWDTREE");

    /* Get pscr-score for fwdtree recognition */
    {
	search_hyp_t *pscrpath;
	
	if (query_phone_conf ()) {
	    pscrpath = fwdtree_pscr_path ();
	    search_hyp_free (pscrpath);
	}
    }
    
#if SEARCH_PROFILE
    if (LastFrame > 0) {
	log_info("%8d words recognized (%d/fr)\n",
		 BPIdx, (BPIdx+(LastFrame>>1))/(LastFrame+1));
	if (topsen_window > 1)
	    log_info("%8d phones in topsen (%d/fr)\n",
		     n_phn_in_topsen, n_phn_in_topsen/(LastFrame+1));
	log_info("%8d senones evaluated (%d/fr)\n", n_senone_active_utt,
		 (n_senone_active_utt + (LastFrame>>1))/(LastFrame+1));
	log_info("%8d channels searched (%d/fr), %d 1st, %d last\n",
		 n_root_chan_eval + n_nonroot_chan_eval,
		 (n_root_chan_eval + n_nonroot_chan_eval)/(LastFrame+1),
		 n_root_chan_eval, n_last_chan_eval);
	log_info("%8d words for which last channels evaluated (%d/fr)\n",
		 n_word_lastchan_eval, n_word_lastchan_eval/(LastFrame+1));
	log_info("%8d candidate words for entering last phone (%d/fr)\n",
		 n_lastphn_cand_utt, n_lastphn_cand_utt/(LastFrame+1));
	
	lm3g_cache_stats_dump (stdout);
    }
#endif
}

void
search_postprocess_bptable (double lwf, char const *pass)
{
    /* register int32 idx; */
    int32 /* i, j, w,*/ cf, nf, f; /*, *awl; */
    /* ROOT_CHAN_T *rhmm; */
    /* CHAN_T *hmm, *thmm, **acl; */
    int32 bp;
    int32 l_scr;
    
    if (LastFrame < 10) {	/* HACK!!  Hardwired constant 10 */
	log_warn("%s(%d): UTTERANCE TOO SHORT; IGNORED\n", __FILE__, __LINE__);
	LastFrame = 0;
	
	return;	
    }
    
    /* Deactivate channels lined up for the next frame */
    cf = CurrentFrame;
    nf = cf+1;
    
    /*
     * Print final Path
     */
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
	if (BPTable[bp].wid == FinishWordId)
	    break;
    }
    if (bp >= BPIdx) {
	int32 bestbp = 0, bestscore = 0; /* FIXME: good defaults? */
	log_warn ("\n%s(%d):  **ERROR**  Failed to terminate in final state\n\n",
		__FILE__, __LINE__);
	/* Find the most recent frame containing the best BP entry */
	for (f = cf; (f >= 0) && (BPTableIdx[f] == BPIdx); --f);
	if (f < 0) {
	    log_warn ("\n%s(%d):  **EMPTY BPTABLE**\n\n", __FILE__, __LINE__);
	    return;
	}
	
	bestscore = WORST_SCORE;
	for (bp = BPTableIdx[f]; bp < BPTableIdx[f+1]; bp++) {
	    l_scr = lm_tg_score (BPTable[bp].prev_real_fwid,
				 BPTable[bp].real_fwid,
				 FinishWordId);
	    l_scr *= lwf;
	    
	    if (BPTable[bp].score+l_scr > bestscore) {
		bestscore = BPTable[bp].score + l_scr;
		bestbp = bp;
	    }
	}
	
	/* Force </s> into bptable */
	CurrentFrame++;
	LastFrame++;
	save_bwd_ptr (FinishWordId, bestscore, bestbp, 0);
	BPTableIdx[CurrentFrame+1] = BPIdx;
	bp = BPIdx-1;
    }
    HypTotalScore = BPTable[bp].score;
    
    compute_seg_scores (lwf);

    seg_back_trace (bp);
    search_remove_context (hyp);
    search_hyp_to_str();

    log_info ("%s: %s (%s %d (A=%d L=%d))\n",
	    pass, hyp_str, uttproc_get_uttid(),
	    HypTotalScore, HypTotalScore - TotalLangScore, TotalLangScore);
}


void bestpath_search ( void )
{
    if (! renormalized) {
	lattice_rescore (bestpath_lw / fwdtree_lw);
	lm3g_cache_stats_dump (stdout);
    } else {
	E_INFO ("Renormalized in fwd pass; cannot rescore lattice\n");
    }
}


/*
 * Convert search hypothesis (word-id sequence) to a single string.
 */
void
search_hyp_to_str ( void )
{
    int32 i, k, l;
    char const *wd;
    
    hyp_str[0] = '\0';
    k = 0;
    for (i = 0; hyp[i].wid >= 0; i++) {
	wd = WordIdToStr (WordDict, hyp[i].wid);
	l = strlen (wd);
	if (k+l > 4090)
	    quit(-1, "%s(%d): **ERROR** Increase hyp_str[] size\n", __FILE__, __LINE__);
	strcpy (hyp_str+k, wd);
	k += l;
	hyp_str[k] = ' ';
	k++;
	hyp_str[k] = '\0';
    }
}


int32 seg_topsen_score (int32 sf, int32 ef)
{
    int32 f, sum;
    
    sum = 0;
    for (f = sf; f <= ef; f++)
	sum += topsen_score[f];
    
    return (sum);
}


/* SEG_BACK_TRACE
 *-------------------------------------------------------------*
 * Print32 out the backtrace
 */
static void
seg_back_trace (int32 bpidx)
{
    static int32 last_score;
    static int32 last_time;
    static int32 seg;
    /* int32 *probs; */
    int32  l_scr;
    int32  a_scr;
    int32  a_scr_norm;		/* time normalized acoustic score */
    int32  raw_scr;
    int32  seg_len;
    int32  altpron;
    int32  topsenscr_norm;
    int32  f, latden;
    double perp;
    
    altpron = query_report_altpron() || ForcedRecMode;
    
    if (bpidx != NO_BP) {
	seg_back_trace (BPTable[bpidx].bp);
	
	l_scr = BPTable[bpidx].lscr;
	raw_scr = BPTable[bpidx].score - last_score;
	a_scr = raw_scr - l_scr;
  	seg_len = BPTable[bpidx].frame - last_time;
	a_scr_norm = ((seg_len == 0) ? 0 : (a_scr / seg_len));
	topsenscr_norm = ((seg_len == 0) ? 0 :
			  seg_topsen_score(last_time+1, BPTable[bpidx].frame) / seg_len);
	
	TotalLangScore += l_scr;
	
	/* Fill in lattice density and perplexity information for this word */
	latden = 0;
	perp = 0.0;
	for (f = last_time+1; f <= BPTable[bpidx].frame; f++) {
	    latden += lattice_density[f];
	    perp += phone_perplexity[f];
	}
	if (seg_len > 0) {
	    latden /= seg_len;
	    perp /= seg_len;
	}
	
	if (print_back_trace)
	    printf("%16s (%4d %4d) %7d %10d %8d %8d %6d %6.2f\n",
		     WordIdToStr(WordDict, BPTable[bpidx].wid),
		     last_time + 1,	BPTable[bpidx].frame,
		     a_scr_norm, a_scr, l_scr,
		     /* BestScoreTable[BPTable[bpidx].frame] -  BPTable[bpidx].score */
		     topsenscr_norm,
		     latden, perp);
	hyp_wid[n_hyp_wid++] = BPTable[bpidx].wid;
	
	/* Store hypothesis word sequence and segmentation */
	if (ISA_REAL_WORD(BPTable[bpidx].wid)) {
	    if (seg >= HYP_SZ-1)
		quit(-1, "%s(%d): **ERROR** Increase HYP_SZ\n", __FILE__, __LINE__);
	    hyp[seg].wid = altpron ? BPTable[bpidx].wid :
		WordDict->dict_list[BPTable[bpidx].wid]->fwid;
	    hyp[seg].sf = uttproc_feat2rawfr(last_time + 1);
	    hyp[seg].ef = uttproc_feat2rawfr(BPTable[bpidx].frame);
	    hyp[seg].ascr = a_scr;
	    hyp[seg].lscr = l_scr;
	    hyp[seg].latden = latden;
	    hyp[seg].phone_perp = perp;
	    seg++;
	    
	    hyp[seg].wid = -1;
        }
	
	last_score = BPTable[bpidx].score;
	last_time = BPTable[bpidx].frame;
    }
    else {
	if (print_back_trace)
	    printf("%16s (%4s %4s) %7s %10s %8s %8s %6s %6s\n\n",
		     "WORD", "SFrm", "Efrm", "AS/Len", "AS_Score", "LM_Scr", "BSDiff",
		     "LatDen", "PhPerp");

	TotalLangScore = 0;
	last_score = 0;
	last_time = -1;		/* Use -1 to count frame 0 */
	seg = 0;
	hyp[0].wid = -1;
	n_hyp_wid = 0;
    }
}

/* PARTIAL_SEG_BACK_TRACE
 *-------------------------------------------------------------*
 * Like seg_back_trace, but not as detailed.
 */
static void
partial_seg_back_trace (int32 bpidx)
{
    static int32 seg;
    static int32 last_time;
    int32  altpron;
    
    altpron = query_report_altpron() || ForcedRecMode;
    
    if (bpidx != NO_BP) {
	partial_seg_back_trace (BPTable[bpidx].bp);
	
	/* Store hypothesis word sequence and segmentation */
	if (ISA_REAL_WORD(BPTable[bpidx].wid)) {
	    if (seg >= HYP_SZ-1)
		quit(-1, "%s(%d): **ERROR** Increase HYP_SZ\n", __FILE__, __LINE__);
	    hyp[seg].wid = altpron ?
		BPTable[bpidx].wid : WordDict->dict_list[BPTable[bpidx].wid]->fwid;
	    hyp[seg].sf = uttproc_feat2rawfr(last_time + 1);
	    hyp[seg].ef = uttproc_feat2rawfr(BPTable[bpidx].frame);
	    seg++;
	    hyp[seg].wid = -1;
        }
	
	last_time = BPTable[bpidx].frame;
    } else {
	last_time = -1;		/* Use -1 to count frame 0 */
	seg = 0;
	hyp[0].wid = -1;
    }
}

static void renormalize_scores (int32 norm)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, **acl;
    int32 i, j, cf, w, *awl;
    
    cf = CurrentFrame;
    
    /* Renormalize root channels */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
	if (rhmm->active == cf) {
	    for (j = 0; j < NODE_CNT; j++)
		if (rhmm->score[j] > WORST_SCORE)
		    rhmm->score[j] -= norm;
	}
    }
    
    /* Renormalize nonroot channels in HMM tree */
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
	for (j = 0; j < NODE_CNT; j++)
	    if (hmm->score[j] > WORST_SCORE)
		hmm->score[j] -= norm;
    }
    
    /* Renormalize individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
	    for (j = 0; j < NODE_CNT; j++)
		if (hmm->score[j] > WORST_SCORE)
		    hmm->score[j] -= norm;
	}
    }
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    for (j = 0; j < NODE_CNT; j++)
		if (rhmm->score[j] > WORST_SCORE)
		    rhmm->score[j] -= norm;
	}
    }

    renormalized = 1;
}

int32 search_get_score (void)
/*---------------------*/
{
    return HypTotalScore;
}


void search_set_beam_width (double beam)
{
    LogBeamWidth =  8 * LOG (beam);
}


/* SEARCH_SET_NEW_WORD_BEAM
 *-------------------------------------------------------------*
 */
void search_set_new_word_beam_width (float beam)
{
    NewWordLogBeamWidth = 8 * LOG (beam);
    log_info ("%8d = new word beam width\n", NewWordLogBeamWidth);
}

/* SEARCH_SET_LASTPHONE_ALONE_BEAM_WIDTH
 *-------------------------------------------------------------*
 */
void search_set_lastphone_alone_beam_width (float beam)
{
    LastPhoneAloneLogBeamWidth = 8 * LOG (beam);
    log_info ("%8d = Last phone alone beam width\n", LastPhoneAloneLogBeamWidth);
}

/* SEARCH_SET_NEW_PHONE_BEAM
 *-------------------------------------------------------------*
 */
void search_set_new_phone_beam_width (float beam)
{
    NewPhoneLogBeamWidth = 8 * LOG (beam);
    log_info ("%8d = new phone beam width\n", NewPhoneLogBeamWidth);
}

/* SEARCH_SET_LAST_PHONE_BEAM
 *-------------------------------------------------------------*
 */
void search_set_last_phone_beam_width (float beam)
{
    LastPhoneLogBeamWidth = 8 * LOG (beam);
    log_info ("%8d = last phone beam width\n", LastPhoneLogBeamWidth);
}

/* SEARCH_SET_CHANNELS_PER_FRAME_TARGET
 *-------------------------------------------------------------*
 */
void search_set_channels_per_frame_target (int32 cpf)
{
     ChannelsPerFrameTarget = cpf;
}

void searchSetScVqTopN (int32 topN)
{
    scVqTopN = topN;
}

int32 searchFrame (void)
{
    return LastFrame;
}

int32 searchCurrentFrame (void)
{
    return CurrentFrame;
}

void
search_set_newword_penalty (double nw_pen)
{
    newword_penalty = LOG (nw_pen);
    log_info ("%8d = newword penalty\n", newword_penalty);
}

void
search_set_silence_word_penalty (float pen,
				 float pip) /* Phone insertion penalty */
{
    logPhoneInsertionPenalty = LOG(pip);
    SilenceWordPenalty = LOG (pen) + LOG (pip);
    log_info ("%8d = LOG (Silence Word Penalty) + LOG (Phone Penalty)\n",
	    SilenceWordPenalty);
}

void
search_set_filler_word_penalty (float pen, float pip)
{
     FillerWordPenalty = LOG (pen) + LOG (pip);;
     log_info ("%8d = LOG (Filler Word Penalty) + LOG (Phone Penalty)\n",
	     FillerWordPenalty);
}

void
search_set_lw (double p1lw, double p2lw, double p3lw)
{
    fwdtree_lw = p1lw;
    fwdflat_lw = p2lw;
    bestpath_lw = p3lw;
    
    log_info ("%s(%d): LW = fwdtree: %.1f, fwdflat: %.1f, bestpath: %.1f\n",
	    __FILE__, __LINE__, fwdtree_lw, fwdflat_lw, bestpath_lw);
}

void
search_set_ip (float ip)
{
    LogInsertionPenalty = LOG(ip);
}

void
search_set_hyp_alternates (int32 arg)
{
    hyp_alternates = arg;
    if (hyp_alternates)
	log_info ("Will report alternate hypotheses\n");
    else
	log_info ("Will NOT report alternate hypotheses\n");
}

void
search_set_skip_alt_frm (int32 flag)
{
    skip_alt_frm = flag;
}

/*
 * SEARCH_FINISH_DOCUMENT: clean up at the end of a "document".
 */
void 
search_finish_document (void)
{
}

void
searchBestN (void)
{
    quit(-1, "%s(%d): searchBestN() not implemented\n", __FILE__,__LINE__);
}

void
search_hyp_write (void)
{
    quit(-1, "%s(%d): search_hyp_write() not implemented\n", __FILE__,__LINE__);
}

void
parse_ref_str (void)
{
    quit(-1, "%s(%d): parse_ref_str() not implemented\n", __FILE__,__LINE__);
}

/*
 * Return the best path in the bptable until now.
 */
int32 search_partial_result (int32 *fr, char **res)
{
    int32 bp, bestscore = 0, bestbp = 0, f; /* FIXME: good defaults? */
    
    bestscore = WORST_SCORE;
    f = CurrentFrame-1;
    
    for (; f >= 0; --f) {
	for (bp = BPTableIdx[f]; bp < BPIdx; bp++) {
	    if (BPTable[bp].score > bestscore) {
		bestscore = BPTable[bp].score;
		bestbp = bp;
	    }
	}
	if (bestscore > WORST_SCORE)
	    break;
    }
    
    if (f >= 0) {
	partial_seg_back_trace (bestbp);
	search_hyp_to_str();
    } else
	hyp_str[0] = '\0';
    
    *fr = uttproc_feat2rawfr(CurrentFrame);
    *res = hyp_str;
    
    return 0;
}

/* SEARCH_RESULT
 *-------------------------------------------------------------*
 * Return the result of the search.
 */
int32 search_result (int32 *fr, char **res)
{
    *fr = uttproc_feat2rawfr(LastFrame);
    *res = hyp_str;
    
    return 0;
}

/*
 * Return recognized word-id sequence; Note that the user cannot clobber it,
 * and must make a copy of it, if needed over the long term.
 */
search_hyp_t *search_get_hyp (void)
{
    return (hyp);
}

char *search_get_wordlist (int *len, char sep_char)
{
    dict_entry_t **dents = WordDict->dict_list;
    int32 dent_cnt = WordDict->dict_entry_count;
    int32 i, p;
    static char *fwrdl = NULL;
    static int32 flen = 0;
    
    /* malloc memory for all word strings in one shot; do it only once */
    if (fwrdl == NULL) {
	for (i = 0, flen = 0; i < dent_cnt; i++)
	    flen += strlen(dents[i]->word) + 1;		/* +1 for sep_char */
	
	++flen;     /* for the terminal '\0' */
	
	fwrdl = (char *) CM_calloc(flen, sizeof(char));
	
	for (i = 0, p = 0; i < dent_cnt; i++) {
	    strcpy(&fwrdl[p], dents[i]->word);
	    p += strlen(dents[i]->word);
	    fwrdl[p++] = sep_char;
	}
	fwrdl[p++] = '\0';
    }
    
    *len = flen;
    return fwrdl;
}

void
search_filtered_endpts (void)
{
    quit(-1, "%s(%d): search_filtered_endpts() not implemented\n", __FILE__,__LINE__);
}

/*
 * Temporary structure for BPTable binary dump.
 */
struct bptbl_entry_s {
    uint16 sf, ef;
    int32 score;
    int32 ascr, lscr;
    uint16 bp;
    uint16 wid;
} bptbl_entry;

void
search_dump_lattice (char const *file)
{
    int32 i;
    FILE *fp;
    
    if ((fp = fopen (file, "w")) == NULL) {
	log_error("%s(%d): fopen(%s,w) failed\n", __FILE__, __LINE__, file);
	return;
    }
    
    for (i = 0; i < BPIdx; i++) {
	bptbl_entry.sf = (BPTable[i].bp >= 0) ? BPTable[BPTable[i].bp].frame+1 : 0;
	bptbl_entry.ef = BPTable[i].frame;
	bptbl_entry.score = BPTable[i].score;
	bptbl_entry.ascr = BPTable[i].ascr;
	bptbl_entry.lscr = BPTable[i].lscr;
	bptbl_entry.bp = BPTable[i].bp;
	bptbl_entry.wid = BPTable[i].wid;
	
	fwrite (&bptbl_entry, sizeof(struct bptbl_entry_s), 1, fp);
    }

    fclose (fp);
}

void
search_dump_lattice_ascii (char const *file)
{
    int32 i, sf;
    FILE *fp;
    
    if ((fp = fopen (file, "w")) == NULL) {
	log_error("%s(%d): fopen(%s,w) failed\n", __FILE__, __LINE__, file);
	return;
    }
    
    fprintf (fp, "%6s %4s %4s %11s %9s %9s %8s %6s %5s %s\n\n",
	     "ID", "SF", "EF", "TOTSCR", "ASCR", "TOPSENSCR", "LSCR", "BPID", "WID", "WORD");
    for (i = 0; i < BPIdx; i++) {
	sf = (BPTable[i].bp >= 0) ? BPTable[BPTable[i].bp].frame+1 : 0;
	
	fprintf (fp, "%6d %4d %4d %11d %9d %9d %8d %6d %5d %s\n",
		 i, sf,
		 BPTable[i].frame,
		 BPTable[i].score,
		 BPTable[i].ascr,
		 seg_topsen_score (sf, BPTable[i].frame),
		 BPTable[i].lscr,
		 BPTable[i].bp,
		 BPTable[i].wid,
		 WordDict->dict_list[BPTable[i].wid]->word);
    }

    fclose (fp);
}


#if SEARCH_TRACE_CHAN_DETAILED
char *trace_wid;

static void load_trace_wordlist (char const *file)
{
    FILE *fp;
    char wd[1000];
    int32 wid;
    
    trace_wid = (char *) CM_calloc (NumWords, sizeof(char));
    log_info("%s(%d): Looking for file trace-wordlist file %s\n",
	     __FILE__, __LINE__, file);
    if ((fp = fopen (file, "r")) == NULL) {
	log_error("%s(%d): fopen(%s,r) failed\n", __FILE__, __LINE__, file);
	return;
    }
    while (fscanf (fp, "%s", wd) == 1) {
	wid = dictStrToWordId (WordDict, wd, TRUE);
	trace_wid[wid] = 1;
    }
    fclose (fp);
}

void
dump_traceword_chan (void)
{
    int32 w, len, i, j, k, cf;
    dict_entry_t *de;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    int32 *sseq_rc;
    
    cf = CurrentFrame;
    for (w = 0; w < NumMainDictWords; w++) {
	if (! trace_wid[w])
	    continue;
	
	de = WordDict->dict_list[w];
	if (de->len == 1)
	    continue;
	
	/* Find root channel for first phone of w */
	for (i = 0, rhmm = root_chan; i < n_root_chan; i++, rhmm++) {
	    if (rhmm->diphone == de->phone_ids[0])
		break;
	}
	if (i >= n_root_chan)
	    quit(-1, "%s(%d): Cannot locate rhmm for %s\n", __FILE__, __LINE__, de->word);
	
	printf ("[%4d] Trace %5d=wid %4d=active[root] %s\n",
		cf, w, rhmm->active, de->word);
	
	/* If root chan active print details */
	if (rhmm->active == cf) {
	    printf ("                   ");
	    printf ("    %10d %10d %10d %10d %10d %10d", rhmm->score[0], rhmm->score[1],
		    rhmm->score[2], rhmm->score[3], rhmm->score[4], rhmm->score[5]);
	    printf ("    %4d %4d %4d %4d %4d %4d\n", rhmm->path[0], rhmm->path[1],
		    rhmm->path[2], rhmm->path[3], rhmm->path[4], rhmm->path[5]);
	    printf ("   ");
	    for (j = 0; j < 5; j++) {
		printf (" %d(", rhmm->sseqid[j]);
		for (k = 0; k < NumCiPhones; k++) {
		    if (LeftContextFwd[rhmm->diphone][k] == rhmm->sseqid[j])
			printf ("%s,", phone_from_id(k));
		}
		printf (")\n");
	    }
	}
	
	/* Track down other phones (except the last) for w in tree */
	for (len = 1; len < de->len-1; len++) {
	    hmm = (len == 1) ? rhmm->next : hmm->next;
	    for (; hmm && (de->phone_ids[len] != hmm->sseqid); hmm = hmm->alt);
	    if (! hmm)
		quit(-1, "%s(%d): Cannot locate %d hmm for %s\n",
		     __FILE__, __LINE__, len, de->word);
	    printf ("    %4d=A %5d=ss", hmm->active, hmm->sseqid);
	    if (hmm->active == cf) {
		printf ("    %10d %10d %10d %10d %10d %10d",
			hmm->score[0], hmm->score[1], hmm->score[2],
			hmm->score[3], hmm->score[4], hmm->score[5]);
		printf ("    %4d %4d %4d %4d %4d %4d\n", hmm->path[0], hmm->path[1],
			hmm->path[2], hmm->path[3], hmm->path[4], hmm->path[5]);
	    } else
		printf ("\n");
	}
	
	/* Track down last phones (with multiple right contexts) */
	for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
	    printf ("    %4d=A          ", hmm->active);
	    printf ("    %10d %10d %10d %10d %10d %10d",
		    hmm->score[0], hmm->score[1], hmm->score[2],
		    hmm->score[3], hmm->score[4], hmm->score[5]);
	    printf ("    %4d %4d %4d %4d %4d %4d", hmm->path[0], hmm->path[1],
		    hmm->path[2], hmm->path[3], hmm->path[4], hmm->path[5]);
	    printf (" %d =ss(", hmm->sseqid);
	    j = de->phone_ids[de->len-1];
	    for (k = 0; k < NumCiPhones; k++) {
		if (RightContextFwd[j][RightContextFwdPerm[j][k]] == hmm->sseqid)
		    printf ("%s,", phone_from_id(k));
	    }
	    printf (")\n");
	}
    }

#if 0
    for (i = 0; i < n_1ph_words; i++) {
	w = single_phone_wid[i];
	if (! trace_wid[w])
	    continue;
	
	de = WordDict->dict_list[w];
	printf ("[%4d] Trace %5d=wid %s\n", cf, w, de->word);
	for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
		printf ("    %4d=A %d=ss", hmm->active, hmm->sseqid);
		printf ("    %10d %10d %10d %10d %10d %10d",
			hmm->score[0], hmm->score[1], hmm->score[2],
			hmm->score[3], hmm->score[4], hmm->score[5]);
		printf ("    %4d %4d %4d %4d %4d %4d\n", hmm->path[0], hmm->path[1],
			hmm->path[2], hmm->path[3], hmm->path[4], hmm->path[5]);
	}
    }
#endif
}
#endif

static int32 *first_phone_rchan_map;	/* map 1st (left) diphone to root-chan index */

/*
 * Allocate that part of the search channel tree structure that is independent of the
 * LM in use.
 */
void
init_search_tree (dictT *dict)
{
    int32 w, mpx, max_ph0, i, s;
    dict_entry_t *de;
    ROOT_CHAN_T *rhmm;
    
    homophone_set = (WORD_ID *) CM_calloc (NumMainDictWords, sizeof (WORD_ID));
    
    /* Find #single phone words, and #unique first diphones (#root channels) in dict. */
    max_ph0 = -1;
    n_1ph_words = 0;
    mpx = dict->dict_list[0]->mpx;
    for (w = 0; w < NumMainDictWords; w++) {
	de = dict->dict_list[w];
	
	if (de->mpx != mpx)
	    quit(-1, "%s(%d): HMM tree words not all mpx or all non-mpx\n",
		 __FILE__, __LINE__);
	
	if (de->len == 1)
	    n_1ph_words++;
	else {
	    if (max_ph0 < de->phone_ids[0])
		max_ph0 = de->phone_ids[0];
	}
    }
    
    /* Add remaining dict words (</s>, <s>, <sil>, noise words) to single-phone words */
    n_1ph_words += (NumWords - NumMainDictWords);
    n_root_chan_alloc = max_ph0+1;
    
    /* Allocate and initialize root channels */
    root_chan = (ROOT_CHAN_T *) CM_calloc (n_root_chan_alloc, sizeof (ROOT_CHAN_T));
    for (i = 0; i < n_root_chan_alloc; i++) {
	root_chan[i].mpx = mpx;
	root_chan[i].penult_phn_wid = -1;
	root_chan[i].active = -1;
	for (s = 0; s < NODE_CNT; s++)
	    root_chan[i].score[s] = WORST_SCORE;
	root_chan[i].bestscore = WORST_SCORE;
	root_chan[i].next = NULL;
    }
    
    /* Allocate space for left-diphone -> root-chan map */
    first_phone_rchan_map = (int32 *) CM_calloc (n_root_chan_alloc, sizeof(int32));
    
    /* Permanently allocate channels for single-phone words (1/word) */
    rhmm = (ROOT_CHAN_T *) CM_calloc (n_1ph_words, sizeof (ROOT_CHAN_T));
    i = 0;
    for (w = 0; w < NumWords; w++) {
	de = WordDict->dict_list[w];
	if (de->len != 1)
	    continue;
	
	rhmm[i].sseqid[0] = de->phone_ids[0];
	rhmm[i].diphone = de->phone_ids[0];
	rhmm[i].ciphone = de->ci_phone_ids[0];
	rhmm[i].mpx = de->mpx;
	rhmm[i].active = -1;
	for (s = 0; s < NODE_CNT; s++)
	    rhmm[i].score[s] = WORST_SCORE;
	rhmm[i].bestscore = WORST_SCORE;
	rhmm[i].next = NULL;
	
	word_chan[w] = (CHAN_T *) &(rhmm[i]);
	i++;
    }
    
    single_phone_wid = (WORD_ID *) CM_calloc (n_1ph_words, sizeof(WORD_ID));

    /*
     * Create search tree once, without using LM, to know the max #nonroot chans.
     * search_initialize() needs this to allocate active-channel lists.
     */
    create_search_tree (dict, 0);	/* arg2=0 => tree for entire dictionary;
					   not just words in the LMs. */
    delete_search_tree ();
}

/*
 * One-time initialization of internal channels in HMM tree.
 */
static void
init_nonroot_chan (CHAN_T *hmm, int32 ph, int32 ci)
{
    int32 s;
    
    hmm->next = NULL;
    hmm->alt = NULL;
    for (s = 0; s < NODE_CNT; s++)
	hmm->score[s] = WORST_SCORE;
    hmm->bestscore = WORST_SCORE;
    hmm->info.penult_phn_wid = -1;
    hmm->active = -1;
    hmm->sseqid = ph;
    hmm->ciphone = ci;
}

/*
 * Allocate and initialize search channel-tree structure.  If (use_lm) do so wrt the
 * currently active LM for the next utterance (i.e., ignore words not in LM).
 * At this point, all the root-channels have been allocated and partly initialized
 * (as per init_search_tree()), and channels for all the single-phone words have been
 * allocated and initialized.  None of the interior channels of search-trees have
 * been allocated.
 * This routine may be called on every utterance, after delete_search_tree() clears
 * the search tree created for the previous utterance.  Meant for reconfiguring the
 * search tree to suit the currently active LM.
 */
void
create_search_tree (dictT *dict, int32 use_lm)
{
    dict_entry_t *de;
    CHAN_T *hmm;
    ROOT_CHAN_T *rhmm;
    int32 w, i, j, p, ph;
    
    if (use_lm)
	log_info("%s(%d): Creating search tree\n", __FILE__, __LINE__);
    else
	log_info("%s(%d): Estimating maximal search tree\n", __FILE__, __LINE__);
    
    for (w = 0; w < NumMainDictWords; w++)
	homophone_set[w] = -1;
    
    for (i = 0; i < n_root_chan_alloc; i++)
	first_phone_rchan_map[i] = -1;
	
    n_1ph_LMwords = 0;
    n_root_chan = 0;
    n_nonroot_chan = 0;
    
    for (w = 0; w < NumMainDictWords; w++) {
	de = dict->dict_list[w];
	
	/* Ignore dictionary words not in LM */
	if (use_lm && (! dictwd_in_lm (de->fwid)))
	    continue;
	
	/* Handle single-phone words individually; not in channel tree */
	if (de->len == 1) {
	    single_phone_wid[n_1ph_LMwords++] = w;
	    continue;
	}
	
	/* Insert w into channel tree; first find or allocate root channel */
	if (first_phone_rchan_map[de->phone_ids[0]] < 0) {
	    first_phone_rchan_map[de->phone_ids[0]] = n_root_chan;
	    rhmm = &(root_chan[n_root_chan]);
	    rhmm->sseqid[0] = de->phone_ids[0];
	    rhmm->diphone = de->phone_ids[0];
	    rhmm->ciphone = de->ci_phone_ids[0];
	    
	    n_root_chan++;
	} else
	    rhmm = &(root_chan[first_phone_rchan_map[de->phone_ids[0]]]);
	
	/* Now, rhmm = root channel for w.  Go on to remaining phones */
	if (de->len == 2) {
	    /* Next phone is the last; not kept in tree; add w to penult_phn_wid set */
	    if ((j = rhmm->penult_phn_wid) < 0)
		rhmm->penult_phn_wid = w;
	    else {
		for (; homophone_set[j] >= 0; j = homophone_set[j]);
		homophone_set[j] = w;
	    }
	} else {
	    /* Add remaining phones, except the last, to tree */
	    ph = de->phone_ids[1];
	    hmm = rhmm->next;
	    if (hmm == NULL) {
		rhmm->next = hmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
		init_nonroot_chan (hmm, ph, de->ci_phone_ids[1]);
		n_nonroot_chan++;
	    } else {
		CHAN_T *prev_hmm = NULL;

		for (; hmm && (hmm->sseqid != ph); hmm = hmm->alt)
		    prev_hmm = hmm;
		if (prev_hmm) {
		    prev_hmm->alt = hmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
		    init_nonroot_chan (hmm, ph, de->ci_phone_ids[1]);
		    n_nonroot_chan++;
		}
	    }
	    /* de->phone_ids[1] now in tree; pointed to by hmm */
	    
	    for (p = 2; p < de->len-1; p++) {
		ph = de->phone_ids[p];
		if (! hmm->next) {
		    hmm->next = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
		    hmm = hmm->next;
		    init_nonroot_chan (hmm, ph, de->ci_phone_ids[p]);
		    n_nonroot_chan++;
		} else {
		    CHAN_T * prev_hmm = NULL;

		    for (hmm = hmm->next; hmm && (hmm->sseqid != ph); hmm = hmm->alt)
			prev_hmm = hmm;
		    if (prev_hmm) {
			prev_hmm->alt = hmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
			init_nonroot_chan (hmm, ph, de->ci_phone_ids[p]);
			n_nonroot_chan++;
		    }
		}
	    }
	    
	    /* All but last phone of w in tree; add w to hmm->info.penult_phn_wid set */
	    if ((j = hmm->info.penult_phn_wid) < 0)
		hmm->info.penult_phn_wid = w;
	    else {
		for (; homophone_set[j] >= 0; j = homophone_set[j]);
		homophone_set[j] = w;
	    }
	}
    }
    
    n_1ph_words = n_1ph_LMwords;
    n_1ph_LMwords++;			/* including </s> */
    
    for (w = FinishWordId; w < NumWords; w++) {
	de = dict->dict_list[w];
	if (use_lm && (! (ISA_FILLER_WORD(w))) && (! dictwd_in_lm (de->fwid)))
	    continue;
	
	single_phone_wid[n_1ph_words++] = w;
    }
    
    if (max_nonroot_chan < n_nonroot_chan+1) {
	/* Give some room for channels for new words added dynamically at run time */
	max_nonroot_chan = n_nonroot_chan+128;
	log_info ("%s(%d): max nonroot chan increased to %d\n",
		__FILE__, __LINE__, max_nonroot_chan);
	
	/* Free old active channel list array if any and allocate new one */
	if (active_chan_list[0] != NULL)
	    free (active_chan_list[0]);
	active_chan_list[0] = (CHAN_T **) CM_calloc (max_nonroot_chan*2, sizeof(CHAN_T *));
	active_chan_list[1] = active_chan_list[0] + max_nonroot_chan;
    }
    
    log_info("%s(%d):   %d root, %d non-root channels, %d single-phone words\n",
	     __FILE__, __LINE__, n_root_chan, n_nonroot_chan, n_1ph_words);
    
#if 0
    printf ("%s(%d): Main Dictionary:\n", __FILE__, __LINE__);
    for (w = 0; w < n_wd; w++) {
	de = dict->dict_list[w];
	printf ("%s", de->word);
	for (i = 0; i < de->len; i++)
	    printf (" %d", de->phone_ids[i]);
	printf ("\n");
    }
    printf ("\n");
    
    printf ("%s(%d): Single-phone words:\n", __FILE__, __LINE__);
    for (i = 0; i < n_1ph_words; i++) {
	printf ("    %5d %s\n",
		single_phone_wid[i], dict->dict_list[single_phone_wid[i]]->word);
    }
    printf ("\n");

    printf ("%s(%d): HMM tree:\n", __FILE__, __LINE__);
    for (i = 0; i < n_root_chan; i++)
	dump_search_tree_root (dict, root_chan+i);
    printf ("\n");
#endif
}

#if 0
int32 mid_stk[100];
static dump_search_tree_root (dictT *dict, ROOT_CHAN_T *hmm)
{
    int32 i;
    CHAN_T *t;
    dict_entry_t *de;
    
    printf (" %d(%d):", hmm->diphone, hmm->mpx);

    for (i = hmm->penult_phn_wid; i >= 0; i = homophone_set[i]) {
	de = dict->dict_list[i];
	printf ("     %d: %s\n", de->phone_ids[de->len-1], de->word);
    }
    
    printf ("    ");
    for (t = hmm->next; t; t = t->alt)
	printf (" %d", t->sseqid);
    printf ("\n");

    mid_stk[0] = hmm->diphone;
    if (hmm->mpx)
	mid_stk[0] |= 0x80000000;
    for (t = hmm->next; t; t = t->alt)
	dump_search_tree (dict, t, 1);
}

static dump_search_tree (dictT *dict, CHAN_T *hmm, int32 level)
{
    int32 i;
    CHAN_T *t;
    dict_entry_t *de;
    
    printf (" %d(%d)", mid_stk[0] & 0x7fffffff, (mid_stk[0] < 0));
    for (i = 1; i < level; i++)
	printf (" %d", mid_stk[i]);
    printf (" %d:\n", hmm->sseqid);

    for (i = hmm->info.penult_phn_wid; i >= 0; i = homophone_set[i]) {
	de = dict->dict_list[i];
	printf ("     %d: %s\n", de->phone_ids[de->len-1], de->word);
    }
    
    printf ("    ");
    for (t = hmm->next; t; t = t->alt)
	printf (" %d", t->sseqid);
    printf ("\n");

    mid_stk[level] = hmm->sseqid;
    for (t = hmm->next; t; t = t->alt)
	dump_search_tree (dict, t, level+1);
}
#endif

/*
 * Delete search tree by freeing all interior channels within search tree and
 * restoring root channel state to the init state (i.e., just after init_search_tree()).
 */
void
delete_search_tree (void)
{
    int32 i;
    CHAN_T *hmm, *sibling;
    
    for (i = 0; i < n_root_chan; i++) {
	hmm = root_chan[i].next;
	
	while (hmm) {
	    sibling = hmm->alt;
	    delete_search_subtree (hmm);
	    hmm = sibling;
	}
	
	root_chan[i].penult_phn_wid = -1;
	root_chan[i].next = NULL;
    }
}

void
delete_search_subtree (CHAN_T *hmm)
{
    CHAN_T *child, *sibling;
    
    /* First free all children under hmm */
    for (child = hmm->next; child; child = sibling) {
	sibling = child->alt;
	delete_search_subtree (child);
    }

    /* Now free hmm */
    listelem_free (hmm, sizeof(CHAN_T));
}

/*
 * Switch search module to new LM.  NOTE: The LM module should have been switched first.
 */
void
search_set_current_lm (void)
{
    lm_t *lm;
    
    lm = lm_get_current ();
    
    if (LangModel)
	delete_search_tree ();
    create_search_tree (WordDict, 1);
    LangModel = lm;
}

/*
 * Compute acoustic and LM scores for each BPTable entry (segment).
 */
void
compute_seg_scores (double lwf)
{
    int32 bp, start_score;
    BPTBL_T *bpe, *p_bpe;
    int32 *rcpermtab;
    dict_entry_t *de;
    
    for (bp = 0; bp < BPIdx; bp++) {
	
	bpe = &(BPTable[bp]);
	
        if (bpe->bp == NO_BP) {
	    bpe->ascr = bpe->score;
	    bpe->lscr = 0;
	    continue;
	}
	
	de = WordDict->dict_list[bpe->wid];
	p_bpe = &(BPTable[bpe->bp]);
	rcpermtab = (p_bpe->r_diph >= 0) ?
	    RightContextFwdPerm[p_bpe->r_diph] : zeroPermTab;
	start_score = BScoreStack[p_bpe->s_idx + rcpermtab[de->ci_phone_ids[0]]];
	
	if (bpe->wid == SilenceWordId) {
	    bpe->lscr = SilenceWordPenalty;
	} else if (ISA_FILLER_WORD(bpe->wid)) {
	    bpe->lscr = FillerWordPenalty;
	} else {
	    bpe->lscr = lm_tg_score (p_bpe->prev_real_fwid, p_bpe->real_fwid, de->fwid);
	    bpe->lscr *= lwf;
	}
	bpe->ascr = bpe->score - start_score - bpe->lscr;
    }
}

int32 search_get_bptable_size (void)
{
    return (BPIdx);
}

int32 *search_get_lattice_density ( void )
{
    return lattice_density;
}

double *search_get_phone_perplexity ( void )
{
    return phone_perplexity;
}

void search_set_hyp_total_score (int32 score)
{
    HypTotalScore = score;
}

int32 search_get_sil_penalty (void)
{
    return (SilenceWordPenalty);
}

int32 search_get_filler_penalty ( void )
{
    return (FillerWordPenalty);
}

BPTBL_T *search_get_bptable ( void )
{
    return (BPTable);
}

int32 *search_get_bscorestack ( void )
{
    return (BScoreStack);
}

double search_get_lw ( void )
{
    return (fwdtree_lw);
}

/* --------------- Re-process lattice a la fbs6 ----------------- */

static latnode_t *frm_wordlist[MAX_FRAMES];
static int32 *fwdflat_wordlist = NULL;

static int32 n_fwdflat_chan;
static int32 n_fwdflat_words;
static int32 n_fwdflat_word_transition;

static char *expand_word_flag = NULL;
static int32 *expand_word_list = NULL;

#define MIN_EF_WIDTH		4
#define MAX_SF_WIN		25


static void build_fwdflat_wordlist ( void )
{
    int32 i, f, sf, ef, wid, nwd;
    BPTBL_T *bp;
    latnode_t *node, *prevnode, *nextnode;
    dict_entry_t *de;
    
    if (! query_fwdtree_flag()) {
	/* No tree-search run; include all words in expansion list */
	for (i = 0; i < StartWordId; i++)
	    fwdflat_wordlist[i] = i;
	fwdflat_wordlist[i] = -1;

	return;
    }
    
    memset (frm_wordlist, 0, MAX_FRAMES * sizeof(latnode_t *));
    
    for (i = 0, bp = BPTable; i < BPIdx; i++, bp++) {
	sf = (bp->bp < 0) ? 0 : BPTable[bp->bp].frame+1;
	ef = bp->frame;
	wid = bp->wid;
	
	if ((wid >= SilenceWordId) || (wid == StartWordId))
	    continue;
	
	de = WordDict->dict_list[wid];
	for (node = frm_wordlist[sf]; node && (node->wid != wid); node = node->next);

	if (node)
	    node->lef = ef;
	else {
	    /* New node; link to head of list */
	    node = (latnode_t *) listelem_alloc (sizeof(latnode_t));
	    node->wid = wid;
	    node->fef = node->lef = ef;

	    node->next = frm_wordlist[sf];
	    frm_wordlist[sf] = node;
	}
    }

    /* Eliminate "unlikely" words, for which there are too few end points */
    for (f = 0; f <= LastFrame; f++) {
	prevnode = NULL;
	for (node = frm_wordlist[f]; node; node = nextnode) {
	    nextnode = node->next;
	    if ((node->lef - node->fef < MIN_EF_WIDTH) ||
		((node->wid == FinishWordId) && (node->lef < LastFrame-1))) {
		if (! prevnode)
		    frm_wordlist[f] = nextnode;
		else
		    prevnode->next = nextnode;
		listelem_free (node, sizeof(latnode_t));
	    } else
		prevnode = node;
	}
    }

    /* Form overall wordlist for 2nd pass */
    nwd = 0;

    memset (word_active, 0, NumWords * sizeof(int32));
    for (f = 0; f <= LastFrame; f++) {
	for (node = frm_wordlist[f]; node; node = node->next) {
	    if (! word_active[node->wid]) {
		word_active[node->wid] = 1;
		fwdflat_wordlist[nwd++] = node->wid;
	    }
	}
    }
    fwdflat_wordlist[nwd] = -1;

    /*
     * NOTE: fwdflat_wordlist excludes <s>, <sil> and noise words; it includes </s>.
     * That is, it includes anything to which a transition can be made in the LM.
     */
}


void destroy_frm_wordlist ( void )
{
    latnode_t *node, *tnode;
    int32 f;
    
    if (! query_fwdtree_flag())
	return;
    
    for (f = 0; f <= LastFrame; f++) {
	for (node = frm_wordlist[f]; node; node = tnode) {
	    tnode = node->next;
	    listelem_free (node, sizeof(latnode_t));
	}
    }
}


void
build_fwdflat_chan ( void )
{
    int32 i, s, wid, p;
    dict_entry_t *de;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, *prevhmm;
    
    for (i = 0; fwdflat_wordlist[i] >= 0; i++) {
	wid = fwdflat_wordlist[i];
	de = WordDict->dict_list[wid];

	if (de->len == 1)
	    continue;

	assert (de->mpx);
	assert (word_chan[wid] == NULL);
	
	rhmm = (ROOT_CHAN_T *) listelem_alloc (sizeof(ROOT_CHAN_T));
	rhmm->diphone = de->phone_ids[0];
	rhmm->ciphone = de->ci_phone_ids[0];
	rhmm->mpx = 1;
	rhmm->active = -1;
	rhmm->bestscore = WORST_SCORE;
	for (s = 0; s < HMM_LAST_STATE; s++) {
	    rhmm->sseqid[s] = 0;
	    rhmm->score[s] = WORST_SCORE;
	}
	rhmm->sseqid[0] = rhmm->diphone;

	prevhmm = NULL;
	for (p = 1; p < de->len-1; p++) {
	    hmm = (CHAN_T *) listelem_alloc (sizeof(CHAN_T));
	    hmm->sseqid = de->phone_ids[p];
	    hmm->info.rc_id = p+1 - de->len;
	    hmm->active = -1;
	    hmm->bestscore = WORST_SCORE;
	    for (s = 0; s < HMM_LAST_STATE; s++)
		hmm->score[s] = WORST_SCORE;

	    if (prevhmm)
		prevhmm->next = hmm;
	    else
		rhmm->next = hmm;
	    
	    prevhmm = hmm;
	}
	
	alloc_all_rc (wid);
	
	if (prevhmm)
	    prevhmm->next = word_chan[wid];
	else
	    rhmm->next = word_chan[wid];
	word_chan[wid] = (CHAN_T *) rhmm;
    }
}


void
destroy_fwdflat_chan ( void )
{
    int32 i, wid; /*, p; */
    dict_entry_t *de;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, *nexthmm;
    
    for (i = 0; fwdflat_wordlist[i] >= 0; i++) {
	wid = fwdflat_wordlist[i];
	de = WordDict->dict_list[wid];

	if (de->len == 1)
	    continue;

	assert (de->mpx);
	assert (word_chan[wid] != NULL);
	
	rhmm = (ROOT_CHAN_T *) word_chan[wid];
	hmm = rhmm->next;
	listelem_free (rhmm, sizeof(ROOT_CHAN_T));

	for (; hmm; hmm = nexthmm) {
	    nexthmm = hmm->next;
	    listelem_free (hmm, sizeof(CHAN_T));
	}
	
	word_chan[wid] = NULL;
    }
}

void
search_set_fwdflat_bw (double bw, double nwbw)
{
    FwdflatLogBeamWidth = 8*LOG(bw);
    FwdflatLogWordBeamWidth = 8*LOG(nwbw);
    log_info ("%s(%d): Flat-pass bw = %.1e (%d), nwbw = %.1e (%d)\n",
	    __FILE__, __LINE__, bw, FwdflatLogBeamWidth, nwbw, FwdflatLogWordBeamWidth);
}

void
search_fwdflat_start ( void )
{
    int32 i, j, s;
    ROOT_CHAN_T *rhmm;
    
    build_fwdflat_wordlist ();
    build_fwdflat_chan ();
    
    BPIdx = 0;
    BSSHead = 0;
    BPTblOflMsg = 0;
    CurrentFrame = 0;
    
    for (i = 0; i < NumWords; i++)
	WordLatIdx[i] = NO_BP;
    
    /* lm3g_cache_reset (); */

    /* Start search with <s>; word_chan[<s>] is permanently allocated */
    rhmm = (ROOT_CHAN_T *) word_chan[StartWordId];
    rhmm->score[0] = 0;
    for (s = 1; s < HMM_LAST_STATE; s++)
	rhmm->score[s] = WORST_SCORE;
    rhmm->bestscore = WORST_SCORE;
    rhmm->path[0] = NO_BP;
    rhmm->active = 0;		/* Frame in which active */

    active_word_list[0][0] = StartWordId;
    n_active_word[0] = 1;
    
    BestScore = 0;
    renormalized = 0;
    
    for (i = 0; i < NumWords; i++)
	last_ltrans[i].sf = -1;

    hyp_str[0] = '\0';
    hyp[0].wid = -1;

    n_fwdflat_chan = 0;
    n_fwdflat_words = 0;
    n_fwdflat_word_transition = 0;
    n_senone_active_utt = 0;
    
    compute_all_senones = query_compute_all_senones();
    distScores = sc_scores[0];
    
    if (! query_fwdtree_flag()) {
	/* No tree-search run; include all words (upto </s>) in expansion list */
	j = 0;

	for (i = 0; i < StartWordId; i++) {
	    if (dictwd_in_lm (WordDict->dict_list[i]->fwid)) {
		expand_word_list[j] = i;
		expand_word_flag[i] = 1;
		j++;
	    } else
		expand_word_flag[i] = 0;
	}
	expand_word_list[j] = -1;
    }
}

void
search_fwdflat_frame (float *cep, float *dcep, float *dcep_80ms, float *pcep, float *ddcep)
{
    int32 nf, i, j;
    int32 *nawl;
    
    if (! compute_all_senones) {
	compute_fwdflat_senone_active ();
	SCVQScores(distScores, cep, dcep, dcep_80ms, pcep, ddcep);
    } else
	SCVQScores_all(distScores, cep, dcep, dcep_80ms, pcep, ddcep);
    n_senone_active_utt += n_senone_active;

    if (CurrentFrame >= MAX_FRAMES-1)
	return;
    
    BPTableIdx[CurrentFrame] = BPIdx;
    
    /* Need to renormalize? */
    if ((BestScore + (2 * LogBeamWidth)) < WORST_SCORE) {
	log_info("Renormalizing Scores at frame %d, best score %d\n",
		 CurrentFrame, BestScore);
	fwdflat_renormalize_scores (BestScore);
    }
    
    BestScore = WORST_SCORE;
    fwdflat_eval_chan ();
    fwdflat_prune_chan ();
    fwdflat_word_transition ();
    
    /* Create next active word list */
    nf = CurrentFrame+1;
    nawl = active_word_list[nf & 0x1];
    for (i = 0, j = 0; fwdflat_wordlist[i] >= 0; i++) {
	if (word_active[fwdflat_wordlist[i]]) {
	    *(nawl++) = fwdflat_wordlist[i];
	    j++;
	}
    }
    for (i = StartWordId; i < NumWords; i++) {
	if (word_active[i]) {
	    *(nawl++) = i;
	    j++;
	}
    }
    n_active_word[nf & 0x1] = j;
    
    /* This code terminates the loop by updating for the next pass */
    CurrentFrame = nf;
    if (CurrentFrame >= MAX_FRAMES-1) {
	log_warn ("%s(%d): MAX_FRAMES (%d) EXCEEDED; IGNORING REST OF UTTERANCE!!\n",
		__FILE__, __LINE__, MAX_FRAMES);
    }
    
    lm_next_frame ();
}

void
compute_fwdflat_senone_active ( void )
{
    int32 i, cf, w, s, d;
    int32 *awl, *dist;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    
    memset (senone_active_flag, 0, TotalDists * sizeof(char));
    n_senone_active = 0;
    
    cf = CurrentFrame;
    
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];

    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    if (rhmm->mpx) {
		for (s = 0; s < HMM_LAST_STATE; s++) {
		    dist = Models[rhmm->sseqid[s]].dist;
		    d = dist[s*3];
		    if (! senone_active_flag[d]) {
			senone_active_flag[d] = 1;
			senone_active[n_senone_active++] = d;
		    }
		}
	    } else {
		dist = Models[rhmm->sseqid[0]].dist;
		for (s = 0; s < TRANS_CNT; s += 3) {
		    d = dist[s];
		    if (! senone_active_flag[d]) {
			senone_active_flag[d] = 1;
			senone_active[n_senone_active++] = d;
		    }
		}
	    }
	}

	for (hmm = rhmm->next; hmm; hmm = hmm->next) {
	    if (hmm->active == cf) {
		dist = Models[hmm->sseqid].dist;
		for (s = 0; s < TRANS_CNT; s += 3) {
		    d = dist[s];
		    if (! senone_active_flag[d]) {
			senone_active_flag[d] = 1;
			senone_active[n_senone_active++] = d;
		    }
		}
	    }
	}
    }
}

void
fwdflat_eval_chan ( void )
{
    int32 i, cf, w, bestscore;
    int32 *awl;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    
    cf = CurrentFrame;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    bestscore = WORST_SCORE;
    
    n_fwdflat_words += i;
    
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    if (rhmm->mpx)
		root_chan_v_mpx_eval (rhmm);
	    else
		root_chan_v_eval (rhmm);

	    n_fwdflat_chan++;
	}
	if ((bestscore < rhmm->bestscore) && (w != FinishWordId))
	    bestscore = rhmm->bestscore;
	
	for (hmm = rhmm->next; hmm; hmm = hmm->next) {
	    if (hmm->active == cf) {
		chan_v_eval (hmm);
		if (bestscore < hmm->bestscore)
		    bestscore = hmm->bestscore;

		n_fwdflat_chan++;
	    }
	}
    }

    BestScoreTable[cf] = BestScore = bestscore;
}

void
fwdflat_prune_chan ( void )
{
    int32 i, cf, nf, w, s, pip, newscore, thresh, wordthresh;
    int32 *awl;
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm, *nexthmm;
    dict_entry_t *de;
    
    cf = CurrentFrame;
    nf = cf+1;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    memset (word_active, 0, NumWords * sizeof(int32));
    
    thresh = BestScore + FwdflatLogBeamWidth;
    wordthresh = BestScore + FwdflatLogWordBeamWidth;
    pip = logPhoneInsertionPenalty;
    
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	de = WordDict->dict_list[w];
	
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    if (rhmm->bestscore > thresh) {
		rhmm->active = nf;
		word_active[w] = 1;
		
		/* Transitions out of root channel */
		newscore = rhmm->score[HMM_LAST_STATE];
		if (rhmm->next) {
		    assert (de->len > 1);
		    
		    newscore += + pip;
		    if (newscore > thresh) {
			hmm = rhmm->next;
			if (hmm->info.rc_id >= 0) {
			    for (; hmm; hmm = hmm->next) {
				if ((hmm->active < cf) || (hmm->score[0] < newscore)) {
				    hmm->score[0] = newscore;
				    hmm->path[0] = rhmm->path[HMM_LAST_STATE];
				    hmm->active = nf;
				}
			    }
			} else {
			    if ((hmm->active < cf) || (hmm->score[0] < newscore)) {
				hmm->score[0] = newscore;
				hmm->path[0] = rhmm->path[HMM_LAST_STATE];
				hmm->active = nf;
			    }
			}
		    }
		} else {
		    assert (de->len == 1);
		    
		    if (newscore > wordthresh) {
			save_bwd_ptr (w, newscore, rhmm->path[HMM_LAST_STATE], 0);
		    }
		}
	    }
	}
	
	for (hmm = rhmm->next; hmm; hmm = hmm->next) {
	    if (hmm->active >= cf) {
		if (hmm->bestscore > thresh) {
		    hmm->active = nf;
		    word_active[w] = 1;
		    
		    newscore = hmm->score[HMM_LAST_STATE];
		    if (hmm->info.rc_id < 0) {
			newscore += pip;
			if (newscore > thresh) {
			    nexthmm = hmm->next;
			    if (nexthmm->info.rc_id >= 0) {
				for (; nexthmm; nexthmm = nexthmm->next) {
				    if ((nexthmm->active < cf) || (nexthmm->score[0] < newscore)) {
					nexthmm->score[0] = newscore;
					nexthmm->path[0] = hmm->path[HMM_LAST_STATE];
					nexthmm->active = nf;
				    }
				}
			    } else {
				if ((nexthmm->active < cf) || (nexthmm->score[0] < newscore)) {
				    nexthmm->score[0] = newscore;
				    nexthmm->path[0] = hmm->path[HMM_LAST_STATE];
				    nexthmm->active = nf;
				}
			    }
			}
		    } else {
			if (newscore > wordthresh) {
			    save_bwd_ptr (w, newscore, hmm->path[HMM_LAST_STATE], hmm->info.rc_id);
			}
		    }
		} else if (hmm->active != nf) {
		    hmm->bestscore = WORST_SCORE;
		    for (s = 0; s < HMM_LAST_STATE; s++)
			hmm->score[s] = WORST_SCORE;
		}
	    }
	}
    }
}

void
fwdflat_word_transition ( void )
{
    int32 cf, nf, b, thresh, pip, i, w, s, newscore;
    int32 best_silrc_score = 0, best_silrc_bp = 0; /* FIXME: good defaults? */
    BPTBL_T *bp;
    dict_entry_t *de, *newde;
    int32 *rcpermtab, *rcss;
    ROOT_CHAN_T *rhmm;
    int32 *awl;
    double lwf;
    
    cf = CurrentFrame;
    nf = cf+1;
    thresh = BestScore + FwdflatLogBeamWidth;
    pip = logPhoneInsertionPenalty;
    best_silrc_score = WORST_SCORE;
    lwf = fwdflat_lw / fwdtree_lw;
    
    get_expand_wordlist (cf, MAX_SF_WIN);

    for (b = BPTableIdx[cf]; b < BPIdx; b++) {
	bp = BPTable+b;
	WordLatIdx[bp->wid] = NO_BP;
	
	if (bp->wid == FinishWordId)
	    continue;
	
	de = WordDict->dict_list[bp->wid];
	rcpermtab = (bp->r_diph >= 0) ? RightContextFwdPerm[bp->r_diph] : zeroPermTab;
	rcss = BScoreStack + bp->s_idx;
	
	for (i = 0; expand_word_list[i] >= 0; i++) {
	    w = expand_word_list[i];
	    newde = WordDict->dict_list[w];
	    newscore = rcss[rcpermtab[newde->ci_phone_ids[0]]];
	    newscore += lm_tg_score(bp->prev_real_fwid, bp->real_fwid, newde->fwid) * lwf;
	    newscore += pip;
	    
	    if (newscore > thresh) {
		rhmm = (ROOT_CHAN_T *) word_chan[w];
		if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
		    rhmm->score[0] = newscore;
		    rhmm->path[0] = b;
		    if (rhmm->mpx)
			rhmm->sseqid[0] =
			    LeftContextFwd[rhmm->diphone][de->ci_phone_ids[de->len-1]];
		    rhmm->active = nf;

		    word_active[w] = 1;
		}
	    }
	}

	if (best_silrc_score < rcss[rcpermtab[SilencePhoneId]]) {
	    best_silrc_score = rcss[rcpermtab[SilencePhoneId]];
	    best_silrc_bp = b;
	}
    }

    /* Transition to <sil> */
    newscore = best_silrc_score + SilenceWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
	w = SilenceWordId;
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
	    rhmm->score[0] = newscore;
	    rhmm->path[0] = best_silrc_bp;
	    rhmm->active = nf;
	    word_active[w] = 1;
	}
    }
    /* Transition to noise words */
    newscore = best_silrc_score + FillerWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
	for (w = SilenceWordId+1; w < NumWords; w++) {
	    rhmm = (ROOT_CHAN_T *) word_chan[w];
	    if ((rhmm->active < cf) || (rhmm->score[0] < newscore)) {
		rhmm->score[0] = newscore;
		rhmm->path[0] = best_silrc_bp;
		rhmm->active = nf;
		word_active[w] = 1;
	    }
	}
    }

    /* Reset initial channels of words that have become inactive even after word trans. */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	
	if (rhmm->active == cf) {
	    rhmm->bestscore = WORST_SCORE;
	    for (s = 0; s < HMM_LAST_STATE; s++)
		rhmm->score[s] = WORST_SCORE;
	}
    }
}

void
search_fwdflat_finish ( void )
{
    destroy_fwdflat_chan ();
    destroy_frm_wordlist ();
    memset (word_active, 0, NumWords * sizeof(int32));
    
    BPTableIdx[CurrentFrame] = BPIdx;
    CurrentFrame--;	/* Backup to the last Active Frame */
    LastFrame = CurrentFrame;

    search_postprocess_bptable ((double) fwdflat_lw/fwdtree_lw, "FWDFLAT");

#if SEARCH_PROFILE
    log_info("%8d words recognized (%d/fr)\n",
	     BPIdx, (BPIdx+(LastFrame>>1))/(LastFrame+1));
    log_info("%8d senones evaluated (%d/fr)\n", n_senone_active_utt,
	     (n_senone_active_utt + (LastFrame>>1))/(LastFrame+1));
    log_info("%8d channels searched (%d/fr)\n",
	     n_fwdflat_chan, n_fwdflat_chan/(LastFrame+1));
    log_info("%8d words searched (%d/fr)\n",
	     n_fwdflat_words, n_fwdflat_words/(LastFrame+1));
    log_info("%8d word transitions (%d/fr)\n",
	    n_fwdflat_word_transition, n_fwdflat_word_transition/(LastFrame+1));

    lm3g_cache_stats_dump (stdout);
#endif
}


static void fwdflat_renormalize_scores (int32 norm)
{
    ROOT_CHAN_T *rhmm;
    CHAN_T *hmm;
    int32 i, j, cf, w, *awl;
    
    cf = CurrentFrame;
    
    /* Renormalize individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
	rhmm = (ROOT_CHAN_T *) word_chan[w];
	if (rhmm->active == cf) {
	    for (j = 0; j < NODE_CNT; j++)
		if (rhmm->score[j] > WORST_SCORE)
		    rhmm->score[j] -= norm;
	}
	for (hmm = rhmm->next; hmm; hmm = hmm->next) {
	    if (hmm->active == cf) {
		for (j = 0; j < NODE_CNT; j++)
		    if (hmm->score[j] > WORST_SCORE)
			hmm->score[j] -= norm;
	    }
	}
    }

    renormalized = 1;
}

void
get_expand_wordlist (int32 frm, int32 win)
{
    int32 f, sf, ef, nwd;
    latnode_t *node;
    
    if (! query_fwdtree_flag()) {
	n_fwdflat_word_transition += StartWordId;
	return;
    }
    
    sf = frm-win;
    if (sf < 0)
	sf = 0;
    ef = frm+win;
    if (ef > MAX_FRAMES)
	ef = MAX_FRAMES;
    
    memset (expand_word_flag, 0, NumWords);
    nwd = 0;
    
    for (f = sf; f < ef; f++) {
	for (node = frm_wordlist[f]; node; node = node->next) {
	    if (! expand_word_flag[node->wid]) {
		expand_word_list[nwd++] = node->wid;
		expand_word_flag[node->wid] = 1;
	    }
	}
    }
    expand_word_list[nwd] = -1;
    n_fwdflat_word_transition += nwd;
}

void
search_fwdflat_init ( void )
{
    fwdflat_wordlist = (int32 *) CM_calloc (NumWords+1, sizeof(int32));
    expand_word_flag = (char *) CM_calloc (NumWords, 1);
    expand_word_list = (int32 *) CM_calloc (NumWords+1, sizeof(int32));
    
#if 0
    log_info ("%s(%d): MIN_EF_WIDTH = %d, MAX_SF_WIN = %d\n",
	    __FILE__, __LINE__, MIN_EF_WIDTH, MAX_SF_WIN);
#endif
}


/* ------------------ CODE FOR TOP SENONES BASED SEARCH PRUNING ----------------- */

#define DUMP_PHN_TOPSEN_SCR	0

static void compute_phone_perplexity( void )
{
    int32 f, nf, p, sum, prob;
    double pp, sumpp;	/* phone probs */
    double logpp;	/* log phone probs */
    double perp;	/* Perplexity */
    register int32 ts = Table_Size;
    register int16 *at = Addition_Table;
    
    nf = searchFrame();

    for (f = 0; f < nf - topsen_window; f++) {
	/* Find Sum(pscr[p]) over p */
	sum = - (utt_pscr[f][0] << 4);
	for (p = 1; p < NumCiPhones; p++) {
	    prob = - (utt_pscr[f][p] << 4);
	    FAST_ADD(sum, sum, prob, at, ts);
	}
	
	perp = 0.0;
	sumpp = 0.0;
	for (p = 0; p < NumCiPhones; p++) {
	    logpp = (utt_pscr[f][p] << 4);
	    logpp = -logpp;
	    logpp -= sum;
	    logpp *= LOG_BASE;
	    pp = exp(logpp);
	    perp -= pp * logpp;

	    sumpp += pp;
	}
	
	phone_perplexity[f] = perp;
    }
    
    for (; f <= nf; f++)
	phone_perplexity[f] = 1.0;
}


static void topsen_init ( void )
{
    int32 p; /* ,s; */
    char const *phn_name;
    
    npa = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
    npa_frm = (int32 **) CM_2dcalloc (topsen_window, NumCiPhones, sizeof(int32));

    if (topsen_window > 1) {
	filler_phone = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
	for (p = 0; p < NumCiPhones; p++) {
	    phn_name = phone_from_id(p);
	    filler_phone[p] = (phn_name[0] == '+');
	}
    } else {
	/* All phones always potentially active */
	for (p = 0; p < NumCiPhones; p++)
	    npa[p] = 1;
    }
}


static void compute_phone_active (int32 topsenscr, int32 npa_th)
{
    int32 *tmp, i, p, *newlist, thresh;
    uint16 *uttpscrp;
    
    thresh = topsenscr + npa_th;
    
    /* Remove oldest frame from active list and reclaim space */
    for (i = 0; i < NumCiPhones; i++)
	npa[i] -= npa_frm[0][i];
    tmp = npa_frm[0];
    for (i = 0; i < topsen_window-1; i++)
	npa_frm[i] = npa_frm[i+1];
    npa_frm[topsen_window-1] = tmp;
    newlist = tmp;

    /* Compute phones predicted by top senones in current frame */
    memset (newlist, 0, NumCiPhones * sizeof(int32));
    uttpscrp = utt_pscr[n_topsen_frm];
    for (i = 0; i < NumCiPhones; i++) {
	if (bestpscr[i] > thresh)
	    newlist[i] = 1;
	uttpscrp[i] = (-bestpscr[i]) >> 4;
    }
    
    /* Add phones active in current frame to cumulative active phone list */
    for (i = 0; i < NumCiPhones; i++) {
	npa[i] += newlist[i];
	if ((! filler_phone[i]) && npa[i])
	    n_phn_in_topsen++;
    }
    
    if (DUMP_PHN_TOPSEN_SCR) {
	static int16 *pscr = NULL;
	static FILE *pscrfp = NULL;
	char pscrfile[1024];
	int32 scaled_scr;
	
	if (! pscr)
	    pscr = (int16 *) CM_calloc (NumCiPhones, sizeof(int32));

	for (i = 0; i < NumCiPhones; i++)
	    pscr[i] = (int16) 0x8000;
	for (p = 0; p < NumCiPhones; p++) {
	    scaled_scr = bestpscr[p] >> 5;
	    if (pscr[p] < scaled_scr)
		pscr[p] = scaled_scr;
	}

	if (n_topsen_frm == 0) {
	    if (pscrfp)
		fclose (pscrfp);
	    sprintf (pscrfile, "%s.pscr", uttproc_get_uttid());
	    if ((pscrfp = fopen (pscrfile, "w")) == NULL)
		printf ("%s(%d): fopen(%s,w) failed\n", __FILE__, __LINE__, pscrfile);
	}
	
	if (pscrfp)
	    fwrite (pscr, sizeof(uint16), NumCiPhones, pscrfp);
    }
}


uint16 **search_get_uttpscr ( void )
{
    return utt_pscr;
}


typedef struct {
    int32 score;
    int16 sf;
    int16 pred;
} vithist_t;


/* Min frames for each phone in allphone decoding */
#define MIN_ALLPHONE_SEG	3
#define PHONE_TRANS_PROB	0.0001


int32
search_uttpscr2phlat_print ( void )
{
    int32 *pval;
    int32 f, i, p, nf, maxp, best, np;
    int32 *pid;
    
    if (topsen_window == 1)
	return -1;	/* No lattice available */
    
    pval = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
    pid = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
    
    E_INFO ("Phone lattice:\n");
    nf = n_topsen_frm;
    for (f = 0; f < nf; f++) {
	for (p = 0; p < NumCiPhones; p++)
	    pval[p] = -(utt_pscr[f][p] << 4);

	best = (int32)0x80000000;
	np = 0;
	for (i = 0; i < NumCiPhones; i++) {
	    maxp = 0;
	    for (p = 1; p < NumCiPhones; p++) {
		if (pval[p] > pval[maxp])
		    maxp = p;
	    }
	    if (pval[maxp] - (topsen_thresh >> 1) >= best)	/* Why >>1? */
		pid[np++] = maxp;
	    else
		break;
	    if (best < pval[maxp])
		best = pval[maxp];
	    pval[maxp] = (int32)0x80000000;
	}

	printf ("%5d %3d", f, np);
	for (i = 0; i < np; i++)
	    printf (" %s", phone_from_id(pid[i]));
	printf("\n");
    }
    
    free (pval);
    return 0;
}

#ifdef DUMP_VITHIST
static void vithist_dump (FILE *fp, vithist_t **vithist, int32 *pid, int32 nfr, int32 n_state)
{
    int32 i, j;
    
    for (i = 0; i < nfr; i++) {
	fprintf (fp, "Frame %4d\n", i);
	for (j = 0; j < n_state; j++) {
	    fprintf (fp, "\t%3d %4d %10d %3d %s\n", j,
		     vithist[i][j].sf, vithist[i][j].score, vithist[i][j].pred,
		     phone_from_id(pid[j]));
	}
	fprintf (fp, "\n");
    }
    fflush (fp);
}
#endif /* DUMP_VITHIST */

/*
 * Search a given CI-phone state-graph using utt_pscr scores for the best path.
 * Called with a completely initialized vithist matrix; in particular the start state.
 * (Not the most efficient, but can be used to search arbitrary phone-state graphs!)
 */
static search_hyp_t *
search_pscr_path (vithist_t **vithist,	/* properly initialized */
		  char **tmat,		/* State-adjacency matrix [from][to] */
		  int32 *pid,		/* CI-phoneid[0..n_state] */
		  int32 n_state,	/* #States in graph being searched */
		  int32 minseg,		/* Min #frames per phone segment */
		  double tprob,		/* State transition (exit) probability */
		  int32 final_state)	/* Nominally, where search should exit */
{
    int32 i, j, f, tp, newscore, bestscore, bestp, pred_bestp, nseg;
    search_hyp_t *head, *tmp;
    
    if (topsen_window <= 1) {
	E_ERROR("Must use -topsen prediction to use this feature\n");
	return NULL;
    }
    
    tp = LOG(tprob);
    
    /* Search */
    for (f = 0; f < n_topsen_frm; f++) {
	/* Update path scores for current frame state scores */
	for (i = 0; i < n_state; i++) {
	    vithist[f][i].score -= (utt_pscr[f][pid[i]] << 4);
	    
	    /* Propagate to next frame */
	    if (vithist[f][i].sf <= f-minseg) {
		newscore = vithist[f][i].score + tp;
		
		for (j = 0; j < n_state; j++) {
		    if ((i == j) || (! tmat[i][j]))
			continue;
		    
		    if (vithist[f+1][j].score < newscore) {
			vithist[f+1][j].score = newscore;
			vithist[f+1][j].pred = i;
			vithist[f+1][j].sf = f+1;
		    }
		}
	    }
	    if (vithist[f+1][i].score <= vithist[f][i].score) {
		vithist[f+1][i].score = vithist[f][i].score;
		vithist[f+1][i].pred = i;
		vithist[f+1][i].sf = vithist[f][i].sf;
	    }
	}
    }
    
    /* Find proper final state to use */
    if (vithist[n_topsen_frm-1][final_state].pred < 0) {
	E_ERROR("%s: search_pscr_path() didn't end in final state\n", uttproc_get_uttid());
#ifdef DUMP_VITHIST
	vithist_dump (stdout, vithist, pid, n_topsen_frm, n_state);
#endif
	
	bestscore = (int32)0x80000000;
	bestp = -1;
	for (i = 0; i < n_state; i++) {
	    if (vithist[n_topsen_frm-1][i].score > bestscore) {
		bestscore = vithist[n_topsen_frm-1][i].score;
		bestp = i;
	    }
	}
	if ((bestp < 0) || (vithist[n_topsen_frm-1][bestp].score <= WORST_SCORE)) {
	    E_ERROR("%s: search_pscr_path() failed\n", uttproc_get_uttid());
	    return NULL;
	}
	final_state = bestp;
    }
    
    /* Backtrace.  (Hack!! Misuse of search_hyp_t to store phones, rather than words) */
    head = (search_hyp_t *) listelem_alloc (sizeof(search_hyp_t));
    head->wid = pid[final_state];
    head->ef = n_topsen_frm - 1;
    head->next = NULL;
    head->ascr = vithist[n_topsen_frm-1][final_state].score;	/* Fixed later */
    nseg = 1;
    
    bestp = final_state;
    for (f = n_topsen_frm-2; f >= 0; --f) {
	pred_bestp = vithist[f+1][bestp].pred;
	
	if (pred_bestp != bestp) {
	    head->sf = f+1;
	    head->ascr -= (vithist[f][pred_bestp].score + tp);
	    
	    tmp = (search_hyp_t *) listelem_alloc (sizeof(search_hyp_t));
	    tmp->wid = pid[pred_bestp];
	    tmp->ef = f;
	    tmp->next = head;
	    head = tmp;
	    head->ascr = vithist[f][pred_bestp].score;
	    
	    nseg++;
	}
	bestp = pred_bestp;
    }
    head->sf = 0;
    
    return head;
}


static void print_pscr_path (FILE *fp, search_hyp_t *hyp, char const *caption)
{
    search_hyp_t *h;
    int32 pathscore, nf;
    
    if (! hyp) {
	E_ERROR("%s(%s): none\n", caption, uttproc_get_uttid());
	return;
    }
    
    fprintf (fp, "%s(%s):\n", caption, uttproc_get_uttid());

    pathscore = nf = 0;
    for (h = hyp; h; h = h->next) {
	fprintf (fp, "%5d %5d %10d %s\n", h->sf, h->ef, h->ascr, phone_from_id(h->wid));
	pathscore += h->ascr;
	nf = h->ef;
    }
    nf++;
    
    fprintf (fp, "Pathscore(%s (%s)): %d /frame: %d\n",
	     caption, uttproc_get_uttid(), pathscore, (pathscore+(nf>>1))/nf);
    fprintf (fp, "\n");
    fflush (fp);
}


search_hyp_t *search_uttpscr2allphone ( void )
{
    static vithist_t **allphone_vithist = NULL;
    static char **allphone_tmat;
    static int32 *allphone_pid;
    search_hyp_t *allp;
    int32 i, j;
    
    if (allphone_vithist == NULL) {
	allphone_vithist = (vithist_t **) CM_2dcalloc (MAX_FRAMES, NumCiPhones,
						       sizeof (vithist_t));
	allphone_pid = (int32 *) CM_calloc (NumCiPhones, sizeof(int32));
	for (i = 0; i < NumCiPhones; i++)
	    allphone_pid[i] = i;
	
	allphone_tmat = (char **) CM_2dcalloc (NumCiPhones, NumCiPhones, sizeof(char));
	for (i = 0; i < NumCiPhones; i++) {
	    for (j = 0; j < NumCiPhones; j++)
		allphone_tmat[i][j] = 1;
	    allphone_tmat[i][i] = 0;
	}
    }
    
    /* Start search with silencephoneid; all others inactive */
    for (i = 0; i < n_topsen_frm; i++) {
	for (j = 0; j < NumCiPhones; j++) {
	    allphone_vithist[i][j].score = WORST_SCORE;
	    allphone_vithist[i][j].sf = 0;
	    allphone_vithist[i][j].pred = -1;
	}
    }
    allphone_vithist[0][SilencePhoneId].score = 0;
    
    allp = search_pscr_path (allphone_vithist,
			     allphone_tmat,
			     allphone_pid,
			     NumCiPhones,
			     MIN_ALLPHONE_SEG,
			     PHONE_TRANS_PROB,
			     SilencePhoneId);
    
    print_pscr_path (stdout, allp, "Allphone-PSCR");

    return allp;
}


static search_hyp_t *fwdtree_pscr_path ( void )
{
    int32 n_state;
    int32 i, j, s;
    dict_entry_t *de;
    vithist_t **pscr_vithist;
    char **pscr_tmat;
    int32 *pscr_pid;
    search_hyp_t *hyp;
    
    n_state = 0;
    for (i = 0; i < n_hyp_wid; i++) {
	de = WordDict->dict_list[hyp_wid[i]];
	n_state += de->len;
    }
    
    pscr_vithist = (vithist_t **) CM_2dcalloc (MAX_FRAMES, n_state,
					       sizeof(vithist_t));

    pscr_pid = (int32 *) CM_calloc (n_state, sizeof(int32));
    s = 0;
    for (i = 0; i < n_hyp_wid; i++) {
	de = WordDict->dict_list[hyp_wid[i]];
	for (j = 0; j < de->len; j++)
	    pscr_pid[s++] = de->ci_phone_ids[j];
    }
    
    pscr_tmat = (char **) CM_2dcalloc (n_state, n_state, sizeof(char));
    for (i = 1; i < n_state; i++)
	pscr_tmat[i-1][i] = 1;
    
    /* Start search with silencephoneid; all others inactive */
    for (i = 0; i < n_topsen_frm; i++) {
	for (j = 0; j < n_state; j++) {
	    pscr_vithist[i][j].score = WORST_SCORE;
	    pscr_vithist[i][j].sf = 0;
	    pscr_vithist[i][j].pred = -1;
	}
    }
    pscr_vithist[0][0].score = 0;
    
    hyp = search_pscr_path (pscr_vithist,
			    pscr_tmat,
			    pscr_pid,
			    n_state,
			    1,
			    PHONE_TRANS_PROB,
			    n_state-1);
    
    free (pscr_vithist);
    free (pscr_pid);
    free (pscr_tmat);
    
    print_pscr_path (stdout, hyp, "FwdTree-PSCR");
    
    return hyp;
}


/*
 * Search bptable for word wid and return its BPTable index.
 * Start search from the given frame frm.
 * Return value: BPTable index that matched.  If none, -1.
 */
int32 search_bptbl_wordlist (int32 wid, int32 frm)
{
    int32 b, first;

    first = BPTableIdx[frm];
    for (b = BPIdx-1; b >= first; --b) {
	if (wid == WordDict->dict_list[BPTable[b].wid]->fwid)
	    return b;
    }
    return -1;
}


int32 search_bptbl_pred (int32 b)
{
    for (b = BPTable[b].bp; ISA_FILLER_WORD(BPTable[b].wid); b = BPTable[b].bp);
    return (WordDict->dict_list[BPTable[b].wid]->fwid);
}
