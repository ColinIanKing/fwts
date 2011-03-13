/*
 * Copyright (C) 2010-2011 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <sys/types.h>
#include <dirent.h>

#include "fwts.h"

typedef struct {
	uint16_t	pin;
	uint32_t	setting;
} hda_audio_pin_setting;

static int hda_audio_init(fwts_framework *fw)
{
	return FWTS_OK;
}

static int hda_audio_deinit(fwts_framework *fw)
{
	return FWTS_OK;
}
			
static fwts_list *hda_audio_read_pins(fwts_framework *fw, const char *path, const char *file)
{
	FILE *fp;
	fwts_list *settings;
	uint16_t	pin;
	uint32_t	setting;
	hda_audio_pin_setting *pin_setting;
	char name[PATH_MAX];

	snprintf(name, sizeof(name), "%s/%s", path, file);

	if ((fp = fopen(name, "r")) == NULL)
		return NULL;

	if ((settings = fwts_list_new()) == NULL) {
		fclose(fp);
		return NULL;
	}

	while (fscanf(fp, "0x%hx 0x%x\n", &pin, &setting) == 2) {
		if ((pin_setting = malloc(sizeof(hda_audio_pin_setting))) == NULL) {
			fwts_list_free(settings, free);
			fclose(fp);
			return NULL;
		}
		pin_setting->pin = pin;
		pin_setting->setting = setting;
		fwts_list_append(settings, pin_setting);
	}
	fclose(fp);
	
	return settings;
}

static void hda_audio_dump_pins(fwts_framework *fw, const char *config, fwts_list *settings)
{
	fwts_list_link *item;

	if (fwts_list_len(settings) > 0) {
		fwts_log_info(fw, "%s:", config);

		fwts_log_info_verbatum(fw, "  Pin  Setting");
		fwts_list_foreach(item, settings) {
			hda_audio_pin_setting *pin_setting = fwts_list_data(hda_audio_pin_setting *, item);
	
			fwts_log_info_verbatum(fw, "  0x%hx 0x%8.8x", pin_setting->pin, pin_setting->setting);
		}
	} else {
		fwts_log_info(fw, "%s: None Defined.", config);
	}
}

static void hda_audio_dev_info(fwts_framework *fw, const char *label, const char *path, const char *file)
{
	char *info;
	char fullpath[PATH_MAX];

	snprintf(fullpath, sizeof(fullpath), "%s/%s", path, file);

	if ((info = fwts_get(fullpath)) != NULL) {
		fwts_chop_newline(info);
		if (*info)
			fwts_log_info_verbatum(fw, "%-15.15s: %s", label, info);
		free(info);
	}
}

static int hda_audio_check_pins(fwts_framework *fw, const char *path)
{
	fwts_list	*init_pin_configs;
	fwts_list	*driver_pin_configs;
	fwts_list	*user_pin_configs;
	int		warn = 0;

	hda_audio_dev_info(fw, "Vendor Name", path, "vendor_name");
	hda_audio_dev_info(fw, "Vendor ID", path, "vendor_id");
	hda_audio_dev_info(fw, "Model Name", path, "modelname");
	hda_audio_dev_info(fw, "Chip Name", path, "chipname");
	hda_audio_dev_info(fw, "Subsystem ID", path, "subsystem_id");
	hda_audio_dev_info(fw, "Revision ID", path, "revision_id");

	init_pin_configs   = hda_audio_read_pins(fw, path, "init_pin_configs");
	driver_pin_configs = hda_audio_read_pins(fw, path, "driver_pin_configs");
	user_pin_configs   = hda_audio_read_pins(fw, path, "user_pin_configs");

	if (fwts_list_len(init_pin_configs) > 0)
		hda_audio_dump_pins(fw, "BIOS pin configurations", init_pin_configs);

	if (fwts_list_len(driver_pin_configs) > 0) {
		hda_audio_dump_pins(fw, "Driver defined pin configurations", driver_pin_configs);
		warn++;
	}
	
	if (fwts_list_len(user_pin_configs) > 0) {
		hda_audio_dump_pins(fw, "User defined pin configurations", driver_pin_configs);
		warn++;
	}

	if (warn) {
		fwts_log_warning(fw, "BIOS pin configurations had needed software override to make HDA audio work correctly.");
		fwts_log_advice(fw, "The driver or user provided overrides should be corrected in BIOS firmware.");
	} else
		fwts_passed(fw, "Default BIOS pin configurations did not have software override.");

	fwts_list_free(user_pin_configs, free);
	fwts_list_free(driver_pin_configs, free);
	fwts_list_free(init_pin_configs, free);

	return FWTS_OK;
}

static int hda_audio_test1(fwts_framework *fw)
{
	DIR *dir;
	struct dirent *directory;

	if ((dir = opendir("/sys/class/sound/")) == NULL)
		return FWTS_ERROR;

	while ((directory = readdir(dir)) != NULL)
		if (strncmp(directory->d_name, "hw", 2) == 0) {
			char path[PATH_MAX];
			snprintf(path, sizeof(path), "/sys/class/sound/%s", directory->d_name);
			fwts_log_info(fw, "Checking %s", directory->d_name);
			hda_audio_check_pins(fw, path);
			fwts_log_nl(fw);
		}

	closedir(dir);

	return FWTS_OK;
}

static fwts_framework_minor_test hda_audio_tests[] = {
	{ hda_audio_test1, "Check HDA Audio Pin Configs." },
	{ NULL, NULL }
};

static fwts_framework_ops hda_audio_ops = {
	.description = "Check HDA Audio Pin Configs.",
	.init        = hda_audio_init,	
	.deinit      = hda_audio_deinit,
	.minor_tests = hda_audio_tests
};

FWTS_REGISTER(hda_audio, &hda_audio_ops, FWTS_TEST_ANYTIME, FWTS_BATCH);
