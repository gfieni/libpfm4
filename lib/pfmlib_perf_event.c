/*
 * pfmlib_perf_events.c: encode events for perf_event API
 *
 * Copyright (c) 2009 Google, Inc
 * Contributed by Stephane Eranian <eranian@google.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <perfmon/pfmlib_perf_event.h>

#include "pfmlib_priv.h"
#include "pfmlib_perf_event_priv.h"

#define PERF_PROC_FILE	"/proc/sys/kernel/perf_event_paranoid"

#ifdef min
#undef min
#endif
#define min(a, b) ((a) < (b) ? (a) : (b))

/*
 * contains ONLY attributes related to PMU features
 */
static const pfmlib_attr_desc_t perf_event_mods[]={
	PFM_ATTR_B("u", "monitor at user level"),	/* monitor user level */
	PFM_ATTR_B("k", "monitor at kernel level"),	/* monitor kernel level */
	PFM_ATTR_B("h", "monitor at hypervisor level"),	/* monitor hypervisor level */
	PFM_ATTR_SKIP, /* to match index in oerf_event_ext_mods */
	PFM_ATTR_SKIP, /* to match index in oerf_event_ext_mods */
	PFM_ATTR_SKIP, /* to match index in oerf_event_ext_mods */
	PFM_ATTR_SKIP, /* to match index in oerf_event_ext_mods */
	PFM_ATTR_B("mg", "monitor guest execution"),	/* monitor guest level */
	PFM_ATTR_B("mh", "monitor host execution"),	/* monitor host level */
	PFM_ATTR_NULL /* end-marker to avoid exporting number of entries */
};

/*
 * contains all attributes controlled by perf_events. That includes PMU attributes
 * and pure software attributes such as sampling periods
 */
static const pfmlib_attr_desc_t perf_event_ext_mods[]={
	PFM_ATTR_B("u", "monitor at user level"),	  /* monitor user level */
	PFM_ATTR_B("k", "monitor at kernel level"),	  /* monitor kernel level */
	PFM_ATTR_B("h", "monitor at hypervisor level"),	  /* monitor hypervisor level */
	PFM_ATTR_I("period", "sampling period"),     	  /* sampling period */
	PFM_ATTR_I("freq", "sampling frequency (Hz)"),	  /* sampling frequency */
	PFM_ATTR_I("precise", "precise event sampling"),  /* anti-skid mechanism */
	PFM_ATTR_B("excl", "exclusive access"),    	  /* exclusive PMU access */
	PFM_ATTR_B("mg", "monitor guest execution"),	  /* monitor guest level */
	PFM_ATTR_B("mh", "monitor host execution"),	  /* monitor host level */
	PFM_ATTR_I("cpu", "CPU to program"),		  /* CPU to program */
	PFM_ATTR_B("pinned", "pin event to counters"),	  /* pin event  to PMU */
	PFM_ATTR_B("hw_smpl", "enable hardware sampling"),/* enable hw_smpl, not precise IP */

	PFM_ATTR_NULL /* end-marker to avoid exporting number of entries */
};

typedef struct sysfs_pmu_entry {
	char *name;
	int type;
	int flags;
} sysfs_pmu_entry_t;

static sysfs_pmu_entry_t *sysfs_pmus;	/* cache os pmus available in sysfs */
static int sysfs_npmus;			/* number of entries in sysfs_pmus */

static int
pfmlib_check_no_mods(pfmlib_event_desc_t *e)
{
	pfmlib_event_attr_info_t *a;
	int numasks = 0, no_mods = 0;
	int i;

	/*
	 * scan umasks used with the event
	 * for support_no_attr values
	 */
	for (i = 0; i < e->nattrs; i++) {
		a = attr(e, i);

		if (a->ctrl != PFM_ATTR_CTRL_PMU)
			continue;

		if (a->type != PFM_ATTR_UMASK)
			continue;

		numasks++;

		if (a->support_no_mods)
			no_mods++;
	}

	/*
	 * handle the case where some umasks have no_attr
	 * and not others. In that case no_attr has priority
	 */
	if (no_mods && numasks != no_mods) {
		DPRINT("event %s with umasks with and without no_mods (%d) attribute, forcing no_mods\n", e->fstr, no_mods);
	}

	return no_mods;
}
static int
pfmlib_perf_event_encode(void *this, const char *str, int dfl_plm, void *data)
{
	pfm_perf_encode_arg_t arg;
	pfm_perf_encode_arg_t *uarg = data;
	pfmlib_os_t *os = this;
	struct perf_event_attr my_attr, *attr;
	pfmlib_pmu_t *pmu;
	pfmlib_event_desc_t e;
	pfmlib_event_attr_info_t *a;
	size_t orig_sz, asz, sz = sizeof(arg);
	uint64_t ival;
	int has_plm = 0, has_vmx_plm = 0;
	int i, plm = 0, ret, vmx_plm = 0;
	int cpu = -1, pinned = 0;
	int no_mods;

	sz = pfmlib_check_struct(uarg, uarg->size, PFM_PERF_ENCODE_ABI0, sz);
	if (!sz)
		return PFM_ERR_INVAL;

	/* copy input */
	memcpy(&arg, uarg, sz);

	/* pointer to our internal attr struct */
	memset(&my_attr, 0, sizeof(my_attr));
	attr = &my_attr;

	/*
	 * copy user attr to our internal version
	 * size == 0 is interpreted minimal possible
	 * size (ABI_VER0)
	 */

	/* size of attr struct passed by user */
	orig_sz = uarg->attr->size;

	if (orig_sz == 0)
		asz = PERF_ATTR_SIZE_VER0;
	else
		asz = min(sizeof(*attr), orig_sz);

	/*
	 * we copy the user struct to preserve whatever may
	 * have been initialized but that we do not use
	 */
	memcpy(attr, uarg->attr, asz);

	/* restore internal size (just in case we need it) */
	attr->size = sizeof(my_attr);

	/* useful for debugging */
	if (asz != sizeof(*attr))
		__pfm_vbprintf("warning: mismatch attr struct size "
			       "user=%d libpfm=%zu\n", asz, sizeof(*attr));

	memset(&e, 0, sizeof(e));

	e.osid = os->id;
	e.os_data = attr;
	e.dfl_plm = dfl_plm;

	/* after this call, need to call pfmlib_release_event() */
	ret = pfmlib_parse_event(str, &e);
	if (ret != PFM_SUCCESS)
		return ret;

	pmu = e.pmu;

	ret = PFM_ERR_NOTSUPP;
	if (!pmu->get_event_encoding[e.osid]) {
		DPRINT("PMU %s does not support PFM_OS_NONE\n", pmu->name);
		goto done;
	}

	ret = pmu->get_event_encoding[e.osid](pmu, &e);
        if (ret != PFM_SUCCESS)
		goto done;

	no_mods = pfmlib_check_no_mods(&e);

	/*
	 * process perf_event attributes
	 */
	for (i = 0; i < e.nattrs; i++) {

		a = attr(&e, i);

		if (a->ctrl != PFM_ATTR_CTRL_PERF_EVENT)
			continue;

		ival = e.attrs[i].ival;

		/*
		 * if event or umasks do not support any modifiers,
		 * then reject
		 */
		if (no_mods && ival && a->idx != PERF_ATTR_PIN) {
			ret = PFM_ERR_ATTR_VAL;
			goto done;
		}

		switch(a->idx) {
		case PERF_ATTR_U:
			if (ival)
				plm |= PFM_PLM3;
			has_plm = 1;
			break;
		case PERF_ATTR_K:
			if (ival)
				plm |= PFM_PLM0;
			has_plm = 1;
			break;
		case PERF_ATTR_H:
			if (ival)
				plm |= PFM_PLMH;
			has_plm = 1;
			break;
		case PERF_ATTR_PE:
			if (!ival || attr->freq) {
				ret = PFM_ERR_ATTR_VAL;
				goto done;
			}
			attr->sample_period = ival;
			break;
		case PERF_ATTR_FR:
			if (!ival || attr->sample_period) {
				ret = PFM_ERR_ATTR_VAL;
				goto done;
			}
			attr->sample_freq = ival;
			attr->freq = 1;
			break;
		case PERF_ATTR_PR:
			if (ival > 3) {
				ret = PFM_ERR_ATTR_VAL;
				goto done;
			}
			attr->precise_ip = ival;
			break;
		case PERF_ATTR_EX:
			if (ival && !attr->exclusive)
				attr->exclusive = 1;
			break;
		case PERF_ATTR_MG:
			vmx_plm |= PFM_PLM3;
			has_vmx_plm = 1;
			break;
		case PERF_ATTR_MH:
			vmx_plm |= PFM_PLM0;
			has_vmx_plm = 1;
			break;
		case PERF_ATTR_CPU:
			if (ival >= INT_MAX) {
				ret = PFM_ERR_ATTR_VAL;
				goto done;
			}
			cpu = (int)ival;
			break;
		case PERF_ATTR_PIN:
			pinned = (int)!!ival;
			break;
		case PERF_ATTR_HWS:
			attr->precise_ip = (int)!!ival;
			break;
		}
	}
	/*
	 * if no priv level mask was provided
	 * with the event, then use dfl_plm
	 */
	if (!has_plm)
		plm = dfl_plm;

	/* exclude_guest by default */
	if (!has_vmx_plm)
		vmx_plm = PFM_PLM0;
	/*
	 * perf_event plm work by exclusion, so use logical or
	 * goal here is to set to zero any exclude_* not supported
	 * by underlying PMU
	 */
	plm     |= (~pmu->supported_plm) & PFM_PLM_ALL;
	vmx_plm |= (~pmu->supported_plm) & PFM_PLM_ALL;

	attr->exclude_user   = !(plm & PFM_PLM3);
	attr->exclude_kernel = !(plm & PFM_PLM0);
	attr->exclude_hv     = !(plm & PFM_PLMH);
	attr->exclude_guest  = !(vmx_plm & PFM_PLM3);
	attr->exclude_host   = !(vmx_plm & PFM_PLM0);
	attr->pinned	     = pinned;

	__pfm_vbprintf("PERF[type=%x config=0x%"PRIx64" config1=0x%"PRIx64
                       " excl=%d excl_user=%d excl_kernel=%d excl_hv=%d excl_host=%d excl_guest=%d period=%"PRIu64" freq=%d"
                       " precise=%d pinned=%d] %s\n",
			attr->type,
			attr->config,
			attr->config1,
			attr->exclusive,
			attr->exclude_user,
			attr->exclude_kernel,
			attr->exclude_hv,
			attr->exclude_host,
			attr->exclude_guest,
			attr->sample_period,
			attr->freq,
			attr->precise_ip,
			attr->pinned,
			str);

	/*
	 * propagate event index if necessary
	 */
	arg.idx = pfmlib_pidx2idx(e.pmu, e.event);

	/* propagate cpu */
	arg.cpu = cpu;

	/* propagate our changes, that overwrites attr->size */
	memcpy(uarg->attr, attr, asz);

	/* restore user size */
	uarg->attr->size = orig_sz;

	/*
	 * fstr not requested, stop here
	 * or no_mods set
	 */
	ret = PFM_SUCCESS;
	if (!arg.fstr || no_mods) {
		memcpy(uarg, &arg, sz);
		goto done;
	}

	for (i=0; i < e.npattrs; i++) {
		int idx;

		if (e.pattrs[i].ctrl != PFM_ATTR_CTRL_PERF_EVENT)
			continue;

		idx = e.pattrs[i].idx;
		switch (idx) {
		case PERF_ATTR_K:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, !!(plm & PFM_PLM0));
			break;
		case PERF_ATTR_U:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, !!(plm & PFM_PLM3));
			break;
		case PERF_ATTR_H:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, !!(plm & PFM_PLMH));
			break;
		case PERF_ATTR_PR:
		case PERF_ATTR_HWS:
			evt_strcat(e.fstr, ":%s=%d", perf_event_ext_mods[idx].name, attr->precise_ip);
			break;
		case PERF_ATTR_PE:
		case PERF_ATTR_FR:
			if (attr->freq && attr->sample_period)
				evt_strcat(e.fstr, ":%s=%"PRIu64, perf_event_ext_mods[idx].name, attr->sample_period);
			else if (attr->sample_period)
				evt_strcat(e.fstr, ":%s=%"PRIu64, perf_event_ext_mods[idx].name, attr->sample_period);
			break;
		case PERF_ATTR_MG:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, !attr->exclude_guest);
			break;
		case PERF_ATTR_MH:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, !attr->exclude_host);
			break;
		case PERF_ATTR_EX:
			evt_strcat(e.fstr, ":%s=%lu", perf_event_ext_mods[idx].name, attr->exclusive);
			break;
		}
	}

	ret = pfmlib_build_fstr(&e, arg.fstr);
	if (ret == PFM_SUCCESS)
		memcpy(uarg, &arg, sz);

done:
	pfmlib_release_event(&e);
	return ret;
}
/*
 * get OS-specific event attributes
 */
static int
perf_get_os_nattrs(void *this, pfmlib_event_desc_t *e)
{
	pfmlib_os_t *os = this;
	int i, n = 0;

	for (i = 0; os->atdesc[i].name; i++)
		if (!is_empty_attr(os->atdesc+i))
			n++;
	return n;
}

static int
perf_get_os_attr_info(void *this, pfmlib_event_desc_t *e)
{
	pfmlib_os_t *os = this;
	pfmlib_event_attr_info_t *info;
	int i, k, j = e->npattrs;

	for (i = k = 0; os->atdesc[i].name; i++) {
		/* skip padding entries */
		if (is_empty_attr(os->atdesc+i))
			continue;

		info = e->pattrs + j + k;

		info->name = os->atdesc[i].name;
		info->desc = os->atdesc[i].desc;
		info->equiv= NULL;
		info->code = i;
		info->idx  = i; /* namespace-specific index */
		info->type = os->atdesc[i].type;
		info->is_dfl = 0;
		info->ctrl = PFM_ATTR_CTRL_PERF_EVENT;
		k++;
	}
	e->npattrs += k;

	return PFM_SUCCESS;
}

/*
 * old interface, maintained for backward compatibility with earlier versions of the library
 */
int
pfm_get_perf_event_encoding(const char *str, int dfl_plm, struct perf_event_attr *attr, char **fstr, int *idx)
{
	pfm_perf_encode_arg_t arg;
	int ret;

	if (PFMLIB_INITIALIZED() == 0)
		return PFM_ERR_NOINIT;

	/* idx and fstr can be NULL */
	if (!(attr && str))
		return PFM_ERR_INVAL;

	if (dfl_plm & ~(PFM_PLM_ALL))
		return PFM_ERR_INVAL;

	memset(&arg, 0, sizeof(arg));

	/* do not clear attr, some fields may be initialized by caller already, e.g., size */
	arg.attr = attr;
	arg.fstr = fstr;

	ret = pfm_get_os_event_encoding(str, dfl_plm, PFM_OS_PERF_EVENT_EXT, &arg);
	if (ret != PFM_SUCCESS)
		return ret;

	if (idx)
		*idx = arg.idx;

	return PFM_SUCCESS;
}

/*
 * generic perf encoding helper
 */
static int
pfmlib_perf_find_pmu_type_by_name(const char *perf_name, int *type)
{
	char filename[PATH_MAX];
	FILE *fp;
	int ret, tmp;
	int retval = PFM_ERR_NOTFOUND;

	if (!(perf_name && type))
		return PFM_ERR_NOTSUPP;

	snprintf(filename, PATH_MAX, "%s/%s/type", SYSFS_PMU_DEVICES_DIR, perf_name);

	fp = fopen(filename, "r");
	if (!fp)
		return PFM_ERR_NOTSUPP;

	ret = fscanf(fp, "%d", &tmp);

	fclose(fp);

	if (ret == 1) {
		*type = tmp;
		retval = PFM_SUCCESS;
	}

	return retval;
}

/*
 * identify perf_events subdirectory
 * via the presence of the mux interval config file
 * Return:
 * 1 : directory is a perf_events directory (match)
 * 0 : directory is not a perf_events directory (match)
 */
static int
filter_pmu_dir(const struct dirent *d)
{
	char fn[PATH_MAX];

	if (d->d_name[0] == '.')
		return 0;

	if (d->d_type != DT_DIR && d->d_type != DT_LNK)
		return 0;

	snprintf(fn, PATH_MAX, "%s/%s/perf_event_mux_interval_ms", SYSFS_PMU_DEVICES_DIR, d->d_name);

	return !access(fn, F_OK);
}

/*
 * build a cache of PMUs available via sysfs
 * to speedup lookup later on
 */
int
pfm_init_sysfs_pmu_cache(void)
{
	struct dirent **dir_list = NULL;
	int n, i, j, ret;
	int type;

	/* only initialize once (perf vs. perf_ext) */
	if (sysfs_pmus)
		return PFM_SUCCESS;

	n = scandir(SYSFS_PMU_DEVICES_DIR, &dir_list, filter_pmu_dir, NULL);
	if (n == 0) {
		free(dir_list);
		return PFM_ERR_NOTSUPP;
	}

	sysfs_pmus = (sysfs_pmu_entry_t *)malloc(n * sizeof(sysfs_pmu_entry_t));
	if (!sysfs_pmus)
		return PFM_ERR_NOMEM;

	/*
	 * cache perf_event PMU name and type (attr.type)
	 */
	for (i = j = 0; i < n; i++) {
		sysfs_pmus[j].name = dir_list[i]->d_name;

		ret = pfmlib_perf_find_pmu_type_by_name(sysfs_pmus[i].name, &type);
		/* skip PMU if cannot get the type */
		if (ret != PFM_SUCCESS) {
			DPRINT("sysfs_pmus[%d]=%s failed to get PMU type from sysfs\n", j, sysfs_pmus[i].name);
			continue;
		}

		sysfs_pmus[j].type = type;

		DPRINT("sysf_pmus[%d]=%s type=%d\n", j, sysfs_pmus[i].name, sysfs_pmus[i].type);

		j++;
	}

	sysfs_npmus = j;

	free(dir_list);

	return PFM_SUCCESS;
}

static int
pfm_perf_event_os_detect(void *this)
{
	if (access(PERF_PROC_FILE, F_OK))
		return PFM_ERR_NOTSUPP;

	return pfm_init_sysfs_pmu_cache();
}

static int
pfmlib_perf_find_pmu_type(char *pmu_name, int *type)
{
	int i;

	if (!sysfs_pmus)
		return PFM_ERR_NOTFOUND;

	for (i = 0; i < sysfs_npmus; i++) {
		/* for now use exact match, add regexp later */
		if (!strcmp(pmu_name, sysfs_pmus[i].name)) {
			*type = sysfs_pmus[i].type;
			return PFM_SUCCESS;
		}
	}
	DPRINT("perf_find_pmu_type: cannot find PMU %s\n", pmu_name);
	return PFM_ERR_NOTFOUND;
}

/*
 * generic perf encoding helper
 */
int
pfm_perf_find_pmu_type(void *this, int *type)
{
	pfmlib_pmu_t *pmu = this;
	char *p, *s, *q;
	int ret;


	/*
	 * if no perf_name specified, then the best
	 * option is to use TYPE_RAW, i.e., the core PMU
	 * which the caller is running on when invoking
	 * perf_event_open()
	 */
	if (!pmu->perf_name) {
		*type = PERF_TYPE_RAW;
		DPRINT("No perf_name for %s, defaulting to TYPE_RAW\n", pmu->name);
		return PFM_SUCCESS;
	}

	/*
	 * perf_name may be a comma separated list of PMU names
	 * so duplicate to split the string into PMU keywords
	 */
	s = q = strdup(pmu->perf_name);
	if (!s) {
		DPRINT("cannot dup perf_name for %s\n", pmu->perf_name);
		return PFM_ERR_NOTSUPP;
	}

	ret = PFM_ERR_NOTFOUND;

	while ((p = strchr(s, ','))) {

		*p = '\0';

		/* stop at first match */
		ret = pfmlib_perf_find_pmu_type(s, type);
		if (ret  == PFM_SUCCESS)
			break;
		s = p + 1;
	}
	/* only or last element of perf_name */
	if (ret == PFM_ERR_NOTFOUND)
		ret = pfmlib_perf_find_pmu_type(s, type);

	free(q);

	if (ret != PFM_SUCCESS) {
		DPRINT("cannot find perf_events PMU type for %s perf_name=%s using PERF_TYPE_RAW\n", pmu->name, pmu->perf_name);
	}
	return ret;
}

pfmlib_os_t pfmlib_os_perf={
	.name = "perf_event",
	.id = PFM_OS_PERF_EVENT,
	.atdesc = perf_event_mods,
	.detect = pfm_perf_event_os_detect,
	.get_os_attr_info = perf_get_os_attr_info,
	.get_os_nattrs = perf_get_os_nattrs,
	.encode = pfmlib_perf_event_encode,
};

pfmlib_os_t pfmlib_os_perf_ext={
	.name = "perf_event extended",
	.id = PFM_OS_PERF_EVENT_EXT,
	.atdesc = perf_event_ext_mods,
	.detect = pfm_perf_event_os_detect,
	.get_os_attr_info = perf_get_os_attr_info,
	.get_os_nattrs = perf_get_os_nattrs,
	.encode = pfmlib_perf_event_encode,
};


