/**
 * Copyright (C) 2013-2014 Analog Devices, Inc.
 *
 * Licensed under the GPL-2.
 *
 **/
#include <stdio.h>

#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <glib/gthread.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_lines.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <malloc.h>
#include <values.h>
#include <sys/stat.h>
#include <string.h>

#include <iio.h>

#include "../osc.h"
#include "../osc_plugin.h"
#include "../config.h"
#include "../iio_widget.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static struct iio_context *ctx;
static struct iio_device *dev;
static struct iio_device *dev_slave;
static struct iio_device *dev_dds_master;

static gint this_page;
static GtkNotebook *nbook;
static gboolean plugin_detached;
static GtkBuilder *builder;

enum fmcomms2adv_wtype {
	CHECKBOX,
	SPINBUTTON,
	COMBOBOX,
	BUTTON,
};

struct w_info {
	enum fmcomms2adv_wtype type;
	const char * const name;
};

static struct w_info attrs[] = {
	{SPINBUTTON, "adi,agc-adc-large-overload-exceed-counter"},
	{SPINBUTTON, "adi,agc-adc-large-overload-inc-steps"},
	{CHECKBOX, "adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable"},
	{SPINBUTTON, "adi,agc-adc-small-overload-exceed-counter"},
	{SPINBUTTON, "adi,agc-attack-delay-extra-margin-us"},
	{SPINBUTTON, "adi,agc-dig-gain-step-size"},
	{SPINBUTTON, "adi,agc-dig-saturation-exceed-counter"},
	{SPINBUTTON, "adi,agc-gain-update-interval-us"},
	{CHECKBOX, "adi,agc-immed-gain-change-if-large-adc-overload-enable"},
	{CHECKBOX, "adi,agc-immed-gain-change-if-large-lmt-overload-enable"},
	{SPINBUTTON, "adi,agc-inner-thresh-high"},
	{SPINBUTTON, "adi,agc-inner-thresh-high-dec-steps"},
	{SPINBUTTON, "adi,agc-inner-thresh-low"},
	{SPINBUTTON, "adi,agc-inner-thresh-low-inc-steps"},
	{SPINBUTTON, "adi,agc-lmt-overload-large-exceed-counter"},
	{SPINBUTTON, "adi,agc-lmt-overload-large-inc-steps"},
	{SPINBUTTON, "adi,agc-lmt-overload-small-exceed-counter"},
	{SPINBUTTON, "adi,agc-outer-thresh-high"},
	{SPINBUTTON, "adi,agc-outer-thresh-high-dec-steps"},
	{SPINBUTTON, "adi,agc-outer-thresh-low"},
	{SPINBUTTON, "adi,agc-outer-thresh-low-inc-steps"},
	{CHECKBOX, "adi,agc-sync-for-gain-counter-enable"},
	{SPINBUTTON, "adi,aux-adc-decimation"},
	{SPINBUTTON, "adi,aux-adc-rate"},
	{COMBOBOX, "adi,clk-output-mode-select"},
	{SPINBUTTON, "adi,ctrl-outs-enable-mask"},
	{SPINBUTTON, "adi,ctrl-outs-index"},
	{SPINBUTTON, "adi,dc-offset-attenuation-high-range"},
	{SPINBUTTON, "adi,dc-offset-attenuation-low-range"},
	{SPINBUTTON, "adi,dc-offset-count-high-range"},
	{SPINBUTTON, "adi,dc-offset-count-low-range"},
	{SPINBUTTON, "adi,dc-offset-tracking-update-event-mask"},
	{SPINBUTTON, "adi,elna-bypass-loss-mdB"},
	{SPINBUTTON, "adi,elna-gain-mdB"},
	{CHECKBOX, "adi,elna-rx1-gpo0-control-enable"},
	{CHECKBOX, "adi,elna-rx2-gpo1-control-enable"},
	{SPINBUTTON, "adi,elna-settling-delay-ns"},
	{CHECKBOX, "adi,ensm-enable-pin-pulse-mode-enable"},
	{CHECKBOX, "adi,ensm-enable-txnrx-control-enable"},
	{CHECKBOX, "adi,external-rx-lo-enable"},
	{CHECKBOX, "adi,external-tx-lo-enable"},
	{CHECKBOX, "adi,frequency-division-duplex-mode-enable"},
	{SPINBUTTON, "adi,gc-adc-large-overload-thresh"},
	{SPINBUTTON, "adi,gc-adc-ovr-sample-size"},
	{SPINBUTTON, "adi,gc-adc-small-overload-thresh"},
	{SPINBUTTON, "adi,gc-dec-pow-measurement-duration"},
	{CHECKBOX, "adi,gc-dig-gain-enable"},
	{SPINBUTTON, "adi,gc-lmt-overload-high-thresh"},
	{SPINBUTTON, "adi,gc-lmt-overload-low-thresh"},
	{SPINBUTTON, "adi,gc-low-power-thresh"},
	{SPINBUTTON, "adi,gc-max-dig-gain"},
	{COMBOBOX, "adi,gc-rx1-mode"},
	{COMBOBOX, "adi,gc-rx2-mode"},
	{SPINBUTTON, "adi,mgc-dec-gain-step"},
	{SPINBUTTON, "adi,mgc-inc-gain-step"},
	{CHECKBOX, "adi,mgc-rx1-ctrl-inp-enable"},
	{CHECKBOX, "adi,mgc-rx2-ctrl-inp-enable"},
	{COMBOBOX, "adi,mgc-split-table-ctrl-inp-gain-mode"},
	{SPINBUTTON, "adi,rssi-delay"},
	{SPINBUTTON, "adi,rssi-duration"},
	{COMBOBOX, "adi,rssi-restart-mode"},
	{SPINBUTTON, "adi,rssi-wait"},
	{COMBOBOX, "adi,rx-rf-port-input-select"},
	{COMBOBOX, "adi,split-gain-table-mode-enable"},
	{CHECKBOX, "adi,tdd-skip-vco-cal-enable"},
	{CHECKBOX, "adi,tdd-use-dual-synth-mode-enable"},
	{CHECKBOX, "adi,tdd-use-fdd-vco-tables-enable"},
	{SPINBUTTON, "adi,temp-sense-decimation"},
	{SPINBUTTON, "adi,temp-sense-measurement-interval-ms"},
	{SPINBUTTON, "adi,temp-sense-offset-signed"},
	{CHECKBOX, "adi,temp-sense-periodic-measurement-enable"},
	{COMBOBOX, "adi,tx-rf-port-input-select"},
	{CHECKBOX, "adi,update-tx-gain-in-alert-enable"},
	{CHECKBOX, "adi,xo-disable-use-ext-refclk-enable"},
	{SPINBUTTON, "adi,fagc-dec-pow-measurement-duration"},
	{CHECKBOX, "adi,fagc-allow-agc-gain-increase-enable"},
	{SPINBUTTON, "adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt"},
	{SPINBUTTON, "adi,fagc-final-overrange-count"},
	{CHECKBOX, "adi,fagc-gain-increase-after-gain-lock-enable"},
	{COMBOBOX, "adi,fagc-gain-index-type-after-exit-rx-mode"},
	{SPINBUTTON, "adi,fagc-lmt-final-settling-steps"},
	{SPINBUTTON, "adi,fagc-lock-level-gain-increase-upper-limit"},
	{CHECKBOX, "adi,fagc-lock-level-lmt-gain-increase-enable"},
	{SPINBUTTON, "adi,fagc-lp-thresh-increment-steps"},
	{SPINBUTTON, "adi,fagc-lp-thresh-increment-time"},
	{SPINBUTTON, "adi,fagc-lpf-final-settling-steps"},
	{SPINBUTTON, "adi,fagc-optimized-gain-offset"},
	{SPINBUTTON, "adi,fagc-power-measurement-duration-in-state5"},
	{CHECKBOX, "adi,fagc-rst-gla-en-agc-pulled-high-enable"},
	{COMBOBOX, "adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable"},
	{SPINBUTTON, "adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll"},
	{CHECKBOX, "adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable"},
	{COMBOBOX, "adi,fagc-rst-gla-if-en-agc-pulled-high-mode"},
	{CHECKBOX, "adi,fagc-rst-gla-large-adc-overload-enable"},
	{CHECKBOX, "adi,fagc-rst-gla-large-lmt-overload-enable"},
	{SPINBUTTON, "adi,fagc-rst-gla-stronger-sig-thresh-above-ll"},
	{CHECKBOX, "adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable"},
	{SPINBUTTON, "adi,fagc-state-wait-time-ns"},
	{COMBOBOX, "adi,fagc-use-last-lock-level-for-set-gain-enable"},
	{CHECKBOX, "adi,aux-dac-manual-mode-enable"},
	{CHECKBOX, "adi,aux-dac1-active-in-alert-enable"},
	{CHECKBOX, "adi,aux-dac1-active-in-rx-enable"},
	{CHECKBOX, "adi,aux-dac1-active-in-tx-enable"},
	{SPINBUTTON, "adi,aux-dac1-default-value-mV"},
	{SPINBUTTON, "adi,aux-dac1-rx-delay-us"},
	{SPINBUTTON, "adi,aux-dac1-tx-delay-us"},
	{CHECKBOX, "adi,aux-dac2-active-in-alert-enable"},
	{CHECKBOX, "adi,aux-dac2-active-in-rx-enable"},
	{CHECKBOX, "adi,aux-dac2-active-in-tx-enable"},
	{SPINBUTTON, "adi,aux-dac2-default-value-mV"},
	{SPINBUTTON, "adi,aux-dac2-rx-delay-us"},
	{SPINBUTTON, "adi,aux-dac2-tx-delay-us"},
	{SPINBUTTON, "adi,rx-fastlock-delay-ns"},
	{SPINBUTTON, "adi,tx-fastlock-delay-ns"},
	{CHECKBOX, "adi,rx-fastlock-pincontrol-enable"},
	{CHECKBOX, "adi,tx-fastlock-pincontrol-enable"},
	{COMBOBOX, "adi,txmon-1-front-end-gain"},
	{SPINBUTTON, "adi,txmon-1-lo-cm"},
	{COMBOBOX, "adi,txmon-2-front-end-gain"},
	{SPINBUTTON, "adi,txmon-2-lo-cm"},
	{CHECKBOX, "adi,txmon-dc-tracking-enable"},
	{SPINBUTTON, "adi,txmon-delay"},
	{SPINBUTTON, "adi,txmon-duration"},
	{SPINBUTTON, "adi,txmon-high-gain"},
	{SPINBUTTON, "adi,txmon-low-gain"},
	{SPINBUTTON, "adi,txmon-low-high-thresh"},
	{CHECKBOX, "adi,txmon-one-shot-mode-enable"},
	{CHECKBOX, "adi,rx1-rx2-phase-inversion-enable"},
	{COMBOBOX, "bist_prbs"},
	{COMBOBOX, "loopback"},
	{BUTTON, "initialize"},
};

static void update_widget(GtkBuilder *builder, struct w_info *item)
{
	GtkWidget *widget;
	int val;
	long long value;

	widget = GTK_WIDGET(gtk_builder_get_object(builder, item->name));
	val = iio_device_debug_attr_read_longlong(dev, item->name, &value);

	/* check for errors, in case there is a kernel <-> userspace mismatch */
	if (val < 0) {
		printf("%s:%s: error accessing '%s' (%s)\n",
			__FILE__, __func__, item->name, strerror(-val));
		gtk_widget_hide(widget);
		return;
	}

	val = (int) value;
	switch (item->type) {
		case CHECKBOX:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !!val);
			break;
		case BUTTON:
			if (val)
				gtk_button_clicked(GTK_BUTTON(widget));
			break;
		case SPINBUTTON:
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), val);
			break;
		case COMBOBOX:
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), val);
			break;
	}

}

void signal_handler_cb (GtkWidget *widget, gpointer data)
{
	struct w_info *item = data;
	unsigned val;

	switch (item->type) {
		case CHECKBOX:
			val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
			break;
		case BUTTON:
			val = 1;
			break;
		case SPINBUTTON:
			val = (unsigned) gtk_spin_button_get_value(GTK_SPIN_BUTTON (widget));
			break;
		case COMBOBOX:
			val = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
			break;
		default:
			return;
	}

	iio_device_debug_attr_write_longlong(dev, item->name, val);

	if (dev_slave)
		iio_device_debug_attr_write_longlong(dev_slave, item->name, val);

	if (!strcmp(item->name, "initialize")) {
		struct osc_plugin *plugin;
		GSList *node;

		for (node = plugin_list; node; node = g_slist_next(node)) {
			plugin = node->data;
			if (plugin && !strncmp(plugin->name, "FMComms2/3/4", 12)) {
				if (plugin->handle_external_request) {
					g_usleep(1 * G_USEC_PER_SEC);
					plugin->handle_external_request("Reload Settings");
				}
			}
		}
	}
}

static void rx_phase_rotation(struct iio_device *dev, gdouble val)
{
	struct iio_channel *out0, *out1;
	gdouble phase;
	unsigned offset;

	printf("%s %s %f\n", __func__, iio_device_get_name(dev), val);

	phase = val * 2 * M_PI / 360.0;

	/* Set both RX1 and RX2 */
	for (offset = 0; offset <= 2; offset += 2) {
		if (offset == 2) {
			out0 = iio_device_find_channel(dev, "voltage2", false);
			out1 = iio_device_find_channel(dev, "voltage3", false);
		} else {
			out0 = iio_device_find_channel(dev, "voltage0", false);
			out1 = iio_device_find_channel(dev, "voltage1", false);
		}

		if (out1 && out0) {
			iio_channel_attr_write_double(out0, "calibscale", (double) cos(phase));
			iio_channel_attr_write_double(out0, "calibphase", (double) (-1 * sin(phase)));
			iio_channel_attr_write_double(out1, "calibscale", (double) cos(phase));
			iio_channel_attr_write_double(out1, "calibphase", (double) sin(phase));
		}
	}
}

static void tx_phase_rotation(struct iio_device *dev, gdouble val)
{
	long long i, q;

	if (val < 0)
		val += 360.0;

	i = (val * 1000) + 90000;
	q = val * 1000;

	if (i > 360000)
		i -= 360000;

	if (q > 360000)
		q -= 360000;


	printf("%s:%d val %f ,  I = %d, Q = %d \n", __func__, __LINE__,val, (int)i,(int) q);


		iio_device_attr_write_longlong(dev,
				"out_altvoltage0_TX1_I_F1_phase",
				i);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage1_TX1_I_F2_phase",
				i);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage2_TX1_Q_F1_phase",
				q);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage3_TX1_Q_F2_phase",
				q);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage4_TX2_I_F1_phase",
				i);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage5_TX2_I_F2_phase",
			       	i);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage6_TX2_Q_F1_phase",
				q);
		iio_device_attr_write_longlong(dev,
				"out_altvoltage7_TX2_Q_F2_phase",
				q);

}

void near_end_loopback_ctrl(unsigned channel, bool enable)
{

	unsigned tmp;
	struct iio_device *dev = iio_context_find_device(ctx, channel > 3 ?
		"cf-ad9361-lpc" : "cf-ad9361-hpc");

	if (channel > 3)
		channel -= 4;

	if (iio_device_reg_read(dev, 0x80000400 + channel * 0x40, &tmp))
		return;

	if (enable)
		tmp |= 0x800;
	else
		tmp &= ~0x800;


	iio_device_reg_write(dev, 0x80000400 + channel * 0x40, tmp);
}

void get_markers (int *offset, int *mag, int *mag1, int *mag2) /* FIXME */
{
	int ret;
	struct marker_type *markers = NULL;
	const char *device_ref = NULL;

	device_ref = plugin_get_device_by_reference("cf-ad9361-lpc");

	if (device_ref) {
		do {
			ret = plugin_data_capture_with_domain(device_ref, NULL, &markers, XCORR_PLOT);
		} while ((ret == -EBUSY));
	}

	if (markers) {
//		printf("X: %f %f %f\n", markers[0].x,  markers[1].x,  markers[2].x);
//		printf("Y: %f %f %f\n", markers[0].y,  markers[1].y,  markers[2].y);

		*offset = markers[0].x;
		*mag =  markers[0].y;
		*mag1 =  markers[1].y;
		*mag2 =  markers[2].y;


	}

	plugin_data_capture_with_domain(NULL, NULL, &markers, XCORR_PLOT);
}

void __cal_switch_ports_enable_cb (unsigned val)
{
	unsigned lp_slave, lp_master, sw;
	char *rx_port, *tx_port;

	/*
	*  0 DISABLE
	*  1 TX1B_B (HPC) -> RX1C_B (HPC) : BIST_LOOPBACK on A
	*  2 TX1B_A (LPC) -> RX1C_B (HPC) : BIST_LOOPBACK on A
	*  3 TX1B_B (HPC) -> RX1C_A (LPC) : BIST_LOOPBACK on B
	*  4 TX1B_A (LPC) -> RX1C_A (LPC) : BIST_LOOPBACK on B
	*
	*/
	switch (val) {
	default:
	case 0:
		lp_slave = 0;
		lp_master = 0;
		sw = 0;
		tx_port = "A";
		rx_port = "A_BALANCED";
		break;
	case 1:
	case 2:
		lp_slave = 0;
		lp_master = 1;
		sw = val - 1;
		tx_port = "B";
		rx_port = "C_BALANCED";
		break;
	case 3:
	case 4:
		lp_slave = 1;
		lp_master = 0;
		sw = val - 1;
		tx_port = "B";
		rx_port = "C_BALANCED";
		break;
	}

	near_end_loopback_ctrl(0, lp_slave); /* HPC */
	near_end_loopback_ctrl(1, lp_slave); /* HPC */

	near_end_loopback_ctrl(4, lp_master); /* LPC */
	near_end_loopback_ctrl(5, lp_master); /* LPC */

	iio_device_debug_attr_write_longlong(dev, "calibration_switch_control", sw);
	iio_device_attr_write(dev, "in_voltage0_rf_port_select", rx_port);
	iio_device_attr_write(dev_slave, "in_voltage0_rf_port_select", rx_port);
	iio_device_attr_write(dev, "out_voltage0_rf_port_select", tx_port);
	iio_device_attr_write(dev_slave, "out_voltage0_rf_port_select", tx_port);

	return;

}

void cal_switch_ports_enable_cb (GtkWidget *widget, gpointer data)
{
	__cal_switch_ports_enable_cb(gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
}

void mcs_cb (GtkWidget *widget, gpointer data)
{
	unsigned step;
	char temp[40], ensm_mode[40];
	struct iio_channel *out0;

	iio_device_attr_read(dev, "ensm_mode", ensm_mode, sizeof(ensm_mode));

	/* Move the parts int ALERT for MCS */
	iio_device_attr_write(dev, "ensm_mode", "alert");
	iio_device_attr_write(dev_slave, "ensm_mode", "alert");

	for (step = 1; step <= 5; step++) {
		sprintf(temp, "%d", step);
		/* Don't change the order here - the master controls the SYNC GPIO */
		iio_device_debug_attr_write(dev_slave, "multichip_sync", temp);
		iio_device_debug_attr_write(dev, "multichip_sync", temp);
		sleep(0.1);
	}

	iio_device_attr_write(dev, "ensm_mode", ensm_mode);
	iio_device_attr_write(dev_slave, "ensm_mode", ensm_mode);

	/* The two DDSs are synced by toggling the ENABLE bit */
	if (dev_dds_master) {
		out0 = iio_device_find_channel(dev_dds_master, "altvoltage0", true);

		iio_channel_attr_write_bool(out0, "raw", 0);
		iio_channel_attr_write_bool(out0, "raw", 1);
	}
}

double calc_phase_offset(unsigned type, double fsample, double dds_freq, int offset, int mag)
{
	double val;

	val = 360.0 / ((fsample / dds_freq) / offset);

	if (mag < 0)
		val += 180.0;

	if (val > 360.0)
		val -= 360.0;


	if (val < 0)
		val = 360.0 + val;

	printf("%s:%d offset = %d, mag = %d, phase %f \n", __func__, __LINE__, offset , mag, val);

	return val;
}

void calibrate (gpointer button)
{
	int offset, mag, i, j, mag1, mag2, delta = 999999999, pdelta, min_delta;
	double rx_phase_lpc, rx_phase_hpc, tx_phase_lpc, tx_phase_hpc, tmp, min_phase;
	struct iio_device *fmc[2];

	GtkWidget *tx_phase;

	tx_phase = GTK_WIDGET(gtk_builder_get_object(builder, "tx_phase"));
	mcs_cb(NULL, NULL);

	fmc[0] = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
	fmc[1] = iio_context_find_device(ctx, "cf-ad9361-dds-core-hpc");

	if (!fmc[0] || !fmc[1]) {
		printf("could not fine dds cores\n");
		goto calibrate_fail;
	}

	/* set some logical defaults / assumptions */
#define CAL_TONE 1000000
#define CAL_SCALE 0.12500
#define CAL_PHASE 0
#define CAL_FREQ 30720000

	for (i = 0; i <= 1; i++) {
		printf("%s : %i\n", __func__, i);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage0_TX1_I_F1_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage0_TX1_I_F1_phase",
				CAL_PHASE + 90000);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage0_TX1_I_F1_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage1_TX1_I_F2_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage1_TX1_I_F2_phase",
				CAL_PHASE + 90000);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage1_TX1_I_F2_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage2_TX1_Q_F1_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage2_TX1_Q_F1_phase",
				CAL_PHASE);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage2_TX1_Q_F1_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage3_TX1_Q_F2_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage3_TX1_Q_F2_phase",
				CAL_PHASE);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage3_TX1_Q_F2_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage4_TX2_I_F1_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage4_TX2_I_F1_phase",
				CAL_PHASE + 90000);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage4_TX2_I_F1_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage5_TX2_I_F2_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage5_TX2_I_F2_phase",
			       	CAL_PHASE + 90000);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage5_TX2_I_F2_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage6_TX2_Q_F1_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage6_TX2_Q_F1_phase",
				CAL_PHASE);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage6_TX2_Q_F1_scale",
				CAL_SCALE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage7_TX2_Q_F2_frequency",
				CAL_TONE);
		iio_device_attr_write_longlong(fmc[i],
				"out_altvoltage7_TX2_Q_F2_phase",
				CAL_PHASE);
		iio_device_attr_write_double(fmc[i],
				"out_altvoltage7_TX2_Q_F2_scale",
				CAL_SCALE);
	}


	/* Toggle RAW on master to sync */
	iio_device_attr_write_longlong(fmc[0],
			"out_altvoltage1_TX1_I_F2_raw", 0);
	iio_device_attr_write_longlong(fmc[0],
			"out_altvoltage1_TX1_I_F2_raw", 1);

	j = 0;
restart:
	j++;
	printf("restarting %i times\n", j);
 	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-lpc"), 0.0);
 	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), 0.0);

	/* Calibrate RX
	 * 1 TX1B_B (HPC) -> RX1C_B (HPC) : BIST_LOOPBACK on A
	 */
	__cal_switch_ports_enable_cb(1);
	plugin_data_capture_revert_xcorr(false);
	sleep(0.1);

	min_delta = 999999999;
	for (i = 0; i < 20; i++) {

		get_markers(&offset, &mag, &mag1, &mag2);
		get_markers(&offset, &mag, &mag1, &mag2);
		if (i == 0) {
			rx_phase_hpc = calc_phase_offset(1, CAL_FREQ, CAL_TONE, offset, mag);
			rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), rx_phase_hpc);
			continue;
		}

		if (i > 0) {
			if (offset != 0) {
				printf("offset %d\n", offset);
				goto restart;
			}
		}

		pdelta = delta;
		delta = abs(mag1) - abs(mag2);

		if (delta < min_delta) {
			min_delta = delta;
			min_phase = rx_phase_hpc;
		}

//		printf("pdelta %d delta %d rx_phase_hpc %f\n",pdelta, delta, rx_phase_hpc);

		if (pdelta > delta)
			rx_phase_hpc -= 0.75;
		else
			rx_phase_hpc += 0.75;

		rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), rx_phase_hpc);


	}
	rx_phase_hpc = min_phase;

	printf("rx_phase_hpc %f\n", rx_phase_hpc);


	/* revert */
	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), 0.0);

	/* Calibrate RX
	 * 3 TX1B_B (HPC) -> RX1C_A (LPC) : BIST_LOOPBACK on B
	 */

	__cal_switch_ports_enable_cb(3);
	plugin_data_capture_revert_xcorr(true);
	sleep(0.1);

	delta = 999999999;
	min_delta = 999999999;

	for (i = 0; i < 20; i++) {

		get_markers(&offset, &mag, &mag1, &mag2);
		get_markers(&offset, &mag, &mag1, &mag2);
		if (i == 0) {
			rx_phase_lpc = calc_phase_offset(3, CAL_FREQ, CAL_TONE, offset, mag);
			rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-lpc"), rx_phase_lpc);
			continue;
		}

		if (i > 0) {
			if (offset != 0) {
				printf("offset %d\n", offset);
				goto restart;
			}
		}

		pdelta = delta;
		delta = abs(mag1) - abs(mag2);

		if (delta < min_delta) {
			min_delta = delta;
			min_phase = rx_phase_lpc;
		}

		//printf("pdelta %d delta %d rx_phase_lpc %f\n",pdelta, delta, rx_phase_lpc);


		if (pdelta > delta)
			rx_phase_lpc -= 0.75;
		else
			rx_phase_lpc += 0.75;


		rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-lpc"), rx_phase_lpc);

	}
	rx_phase_lpc = min_phase;

	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), rx_phase_hpc);

	printf("rx_phase_lpc %f\n", rx_phase_lpc);

#if 0
	/* Calibrate TX
	 * 2 TX1B_A (LPC) -> RX1C_B (HPC) : BIST_LOOPBACK on A
	 */
	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-lpc"), 0.0);
	__cal_switch_ports_enable_cb(2);
	plugin_data_capture_revert_xcorr(false);

	sleep(0.2);
	get_markers(&offset, &mag, &mag1, &mag2);
	get_markers(&offset, &mag, &mag1, &mag2);
	tx_phase_lpc = calc_phase_offset(2, CAL_FREQ, CAL_TONE, offset, mag);
	printf("tx_phase_lpc %f\n", tx_phase_lpc);
#endif

	/* Calibrate TX
	 * 4 TX1B_A (LPC) -> RX1C_A (LPC) : BIST_LOOPBACK on B
	 */

	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), 0.0);
	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-lpc"), rx_phase_lpc);
	__cal_switch_ports_enable_cb(4);
	plugin_data_capture_revert_xcorr(true);
	sleep(0.1);
	get_markers(&offset, &mag, &mag1, &mag2);
	get_markers(&offset, &mag, &mag1, &mag2);
	tx_phase_hpc = calc_phase_offset(4, CAL_FREQ, CAL_TONE, offset, mag);

	printf("tx_phase_hpc %f\n", tx_phase_hpc);

	rx_phase_rotation(iio_context_find_device(ctx, "cf-ad9361-hpc"), rx_phase_hpc);

	tmp = -1 * tx_phase_hpc;

	if (tmp > 360.0)
		tmp -= 360.0;

	if (tmp < 0.0)
		tmp += 360.0;

//	tx_phase_rotation(fmc[1], tmp);

	gtk_range_set_value(GTK_RANGE(tx_phase), tmp);


	plugin_data_capture_revert_xcorr(false);
	 __cal_switch_ports_enable_cb(0);

calibrate_fail:
	gdk_threads_enter();
	gtk_widget_show(GTK_WIDGET(button));
	gdk_threads_leave();

	g_thread_exit(NULL);
}



void do_calibration (GtkWidget *widget, gpointer data)
{
	g_thread_new("Calibrate_thread", (void *) &calibrate, data);
	gtk_widget_hide(GTK_WIDGET(data));
}


void tx_phase_hscale_value_changed (GtkRange *hscale1, gpointer data)
{
	static struct iio_device *fmc[2] = {NULL, NULL};

	double value = gtk_range_get_value(hscale1);

	if (fmc[0] == NULL)
		fmc[0] = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
	if (fmc[1] == NULL)
		fmc[1] = iio_context_find_device(ctx, "cf-ad9361-dds-core-hpc");

	if ((int)data)
		tx_phase_rotation(fmc[0], value);
	else
		tx_phase_rotation(fmc[1], value);

	/* Toggle RAW on master to sync */
	iio_device_attr_write_longlong(fmc[0],
			"out_altvoltage1_TX1_I_F2_raw", 0);
	iio_device_attr_write_longlong(fmc[0],
			"out_altvoltage1_TX1_I_F2_raw", 1);

}

void bist_tone_cb (GtkWidget *widget, gpointer data)
{
	GtkBuilder *builder = data;
	unsigned mode, level, freq, c2i, c2q, c1i, c1q;
	char temp[40];

	mode = gtk_combo_box_get_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "bist_tone"))));
	level = gtk_combo_box_get_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "tone_level"))));
	freq = gtk_combo_box_get_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "bist_tone_frequency"))));
	c2i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		GTK_WIDGET(gtk_builder_get_object(builder, "c2i"))));
	c2q = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		GTK_WIDGET(gtk_builder_get_object(builder, "c2q"))));
	c1i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		GTK_WIDGET(gtk_builder_get_object(builder, "c1i"))));
	c1q = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
		GTK_WIDGET(gtk_builder_get_object(builder, "c1q"))));

	sprintf(temp, "%d %d %d %d", mode, freq, level * 6,
		(c2q << 3) | (c2i << 2) | (c1q << 1) | c1i);

	iio_device_debug_attr_write(dev, "bist_tone", temp);

	if (dev_slave)
		iio_device_debug_attr_write(dev_slave, "bist_tone", temp);

}

static void connect_widget(GtkBuilder *builder, struct w_info *item)
{
	GtkWidget *widget;
	char *signal = NULL;
	int val;
	long long value;

	widget = GTK_WIDGET(gtk_builder_get_object(builder, item->name));
	val = iio_device_debug_attr_read_longlong(dev, item->name, &value);

	/* check for errors, in case there is a kernel <-> userspace mismatch */
	if (val < 0) {
		printf("%s:%s: error accessing '%s' (%s)\n",
			__FILE__, __func__, item->name, strerror(-val));
		gtk_widget_hide(widget);
		return;
	}

	val = (int) value;
	switch (item->type) {
		case CHECKBOX:
			signal = "toggled";
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !!val);
			break;
		case BUTTON:
			signal = "clicked";
			break;
		case SPINBUTTON:
			signal = "value-changed";
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), val);
			break;
		case COMBOBOX:
			signal = "changed";
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), val);
			break;
	}

	g_builder_connect_signal(builder, item->name, signal,
		G_CALLBACK(signal_handler_cb), item);
}

void change_page_cb (GtkNotebook *notebook, GtkNotebookPage *page,
		     guint page_num, gpointer user_data)
{
	GtkWidget *tohide = user_data;

	if (page_num == 7)
		gtk_widget_hide(tohide); /* Hide Init button in BIST Tab */
	else
		gtk_widget_show(tohide);
}

static int fmcomms2adv_init(GtkWidget *notebook)
{
	GtkWidget *fmcomms2adv_panel;
	int i;

	builder = gtk_builder_new();
	nbook = GTK_NOTEBOOK(notebook);

	if (!gtk_builder_add_from_file(builder, "fmcomms2_adv.glade", NULL))
		gtk_builder_add_from_file(builder, OSC_GLADE_FILE_PATH "fmcomms2_adv.glade", NULL);

	fmcomms2adv_panel = GTK_WIDGET(gtk_builder_get_object(builder, "fmcomms2adv_panel"));

	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		connect_widget(builder, &attrs[i]);

	gtk_combo_box_set_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "bist_tone"))), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "bist_tone_frequency"))), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "tone_level"))), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "bist_prbs"))), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(
		GTK_WIDGET(gtk_builder_get_object(builder, "loopback"))), 0);

	g_builder_connect_signal(builder, "bist_tone", "changed",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "bist_tone_frequency", "changed",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "tone_level", "changed",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "c2q", "toggled",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "c1q", "toggled",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "c2i", "toggled",
		G_CALLBACK(bist_tone_cb), builder);

	g_builder_connect_signal(builder, "c1i", "toggled",
		G_CALLBACK(bist_tone_cb), builder);

	gtk_combo_box_set_active(
			GTK_COMBO_BOX(gtk_builder_get_object(builder, "calibration_switch_control")), 0);
	__cal_switch_ports_enable_cb(0);

	g_builder_connect_signal(builder, "calibration_switch_control", "changed",
		G_CALLBACK(cal_switch_ports_enable_cb), builder);

	g_builder_connect_signal(builder, "tx_phase", "value-changed",
		G_CALLBACK(tx_phase_hscale_value_changed), 0);


	g_builder_connect_signal(builder, "do_fmcomms5_cal", "clicked",
			G_CALLBACK(do_calibration), gtk_builder_get_object(builder, "do_fmcomms5_cal"));

	if (dev_slave) {
		g_builder_connect_signal(builder, "mcs_sync", "clicked",
			G_CALLBACK(mcs_cb), builder);
	} else {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "mcs_sync")));
	}

	this_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), fmcomms2adv_panel, NULL);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), fmcomms2adv_panel, "FMComms2 Advanced");

	g_builder_connect_signal(builder, "notebook1", "switch-page",
		G_CALLBACK(change_page_cb),
		GTK_WIDGET(gtk_builder_get_object(builder, "initialize")));

	return 0;
}

#define SYNC_RELOAD "SYNC_RELOAD"

static char *handle_item(struct osc_plugin *plugin, const char *attrib,
			 const char *value)
{
	int i;

	if (MATCH_ATTRIB(SYNC_RELOAD)) {
		if (value) {
			for (i = 0; i < ARRAY_SIZE(attrs); i++)
				update_widget(builder, &attrs[i]);
			gtk_button_clicked(GTK_BUTTON(gtk_builder_get_object(builder,
					"initialize")));
		} else {
			return "1";
		}
	} else {
		if (value) {
			printf("Unhandled tokens in ini file,\n"
				"\tSection %s\n\tAtttribute : %s\n\tValue: %s\n",
				"FMComms2 Advanced", attrib, value);
			return "FAIL";
		}
	}

	return NULL;
}

static const char *fmcomms2_adv_sr_attribs[] = {
	"debug.ad9361-phy.adi,agc-adc-large-overload-exceed-counter",
	"debug.ad9361-phy.adi,agc-adc-large-overload-inc-steps",
	"debug.ad9361-phy.adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable",
	"debug.ad9361-phy.adi,agc-adc-small-overload-exceed-counter",
	"debug.ad9361-phy.adi,agc-attack-delay-extra-margin-us",
	"debug.ad9361-phy.adi,agc-dig-gain-step-size",
	"debug.ad9361-phy.adi,agc-dig-saturation-exceed-counter",
	"debug.ad9361-phy.adi,agc-gain-update-interval-us",
	"debug.ad9361-phy.adi,agc-immed-gain-change-if-large-adc-overload-enable",
	"debug.ad9361-phy.adi,agc-immed-gain-change-if-large-lmt-overload-enable",
	"debug.ad9361-phy.adi,agc-inner-thresh-high",
	"debug.ad9361-phy.adi,agc-inner-thresh-high-dec-steps",
	"debug.ad9361-phy.adi,agc-inner-thresh-low",
	"debug.ad9361-phy.adi,agc-inner-thresh-low-inc-steps",
	"debug.ad9361-phy.adi,agc-lmt-overload-large-exceed-counter",
	"debug.ad9361-phy.adi,agc-lmt-overload-large-inc-steps",
	"debug.ad9361-phy.adi,agc-lmt-overload-small-exceed-counter",
	"debug.ad9361-phy.adi,agc-outer-thresh-high",
	"debug.ad9361-phy.adi,agc-outer-thresh-high-dec-steps",
	"debug.ad9361-phy.adi,agc-outer-thresh-low",
	"debug.ad9361-phy.adi,agc-outer-thresh-low-inc-steps",
	"debug.ad9361-phy.adi,agc-sync-for-gain-counter-enable",
	"debug.ad9361-phy.adi,aux-adc-decimation",
	"debug.ad9361-phy.adi,aux-adc-rate",
	"debug.ad9361-phy.adi,clk-output-mode-select",
	"debug.ad9361-phy.adi,ctrl-outs-enable-mask",
	"debug.ad9361-phy.adi,ctrl-outs-index",
	"debug.ad9361-phy.adi,dc-offset-attenuation-high-range",
	"debug.ad9361-phy.adi,dc-offset-attenuation-low-range",
	"debug.ad9361-phy.adi,dc-offset-count-high-range",
	"debug.ad9361-phy.adi,dc-offset-count-low-range",
	"debug.ad9361-phy.adi,dc-offset-tracking-update-event-mask",
	"debug.ad9361-phy.adi,elna-bypass-loss-mdB",
	"debug.ad9361-phy.adi,elna-gain-mdB",
	"debug.ad9361-phy.adi,elna-rx1-gpo0-control-enable",
	"debug.ad9361-phy.adi,elna-rx2-gpo1-control-enable",
	"debug.ad9361-phy.adi,elna-settling-delay-ns",
	"debug.ad9361-phy.adi,ensm-enable-pin-pulse-mode-enable",
	"debug.ad9361-phy.adi,ensm-enable-txnrx-control-enable",
	"debug.ad9361-phy.adi,external-rx-lo-enable",
	"debug.ad9361-phy.adi,external-tx-lo-enable",
	"debug.ad9361-phy.adi,frequency-division-duplex-mode-enable",
	"debug.ad9361-phy.adi,gc-adc-large-overload-thresh",
	"debug.ad9361-phy.adi,gc-adc-ovr-sample-size",
	"debug.ad9361-phy.adi,gc-adc-small-overload-thresh",
	"debug.ad9361-phy.adi,gc-dec-pow-measurement-duration",
	"debug.ad9361-phy.adi,gc-dig-gain-enable",
	"debug.ad9361-phy.adi,gc-lmt-overload-high-thresh",
	"debug.ad9361-phy.adi,gc-lmt-overload-low-thresh",
	"debug.ad9361-phy.adi,gc-low-power-thresh",
	"debug.ad9361-phy.adi,gc-max-dig-gain",
	"debug.ad9361-phy.adi,gc-rx1-mode",
	"debug.ad9361-phy.adi,gc-rx2-mode",
	"debug.ad9361-phy.adi,mgc-dec-gain-step",
	"debug.ad9361-phy.adi,mgc-inc-gain-step",
	"debug.ad9361-phy.adi,mgc-rx1-ctrl-inp-enable",
	"debug.ad9361-phy.adi,mgc-rx2-ctrl-inp-enable",
	"debug.ad9361-phy.adi,mgc-split-table-ctrl-inp-gain-mode",
	"debug.ad9361-phy.adi,rssi-delay",
	"debug.ad9361-phy.adi,rssi-duration",
	"debug.ad9361-phy.adi,rssi-restart-mode",
	"debug.ad9361-phy.adi,rssi-wait",
	"debug.ad9361-phy.adi,rx-rf-port-input-select",
	"debug.ad9361-phy.adi,split-gain-table-mode-enable",
	"debug.ad9361-phy.adi,tdd-skip-vco-cal-enable",
	"debug.ad9361-phy.adi,tdd-use-dual-synth-mode-enable",
	"debug.ad9361-phy.adi,tdd-use-fdd-vco-tables-enable",
	"debug.ad9361-phy.adi,temp-sense-decimation",
	"debug.ad9361-phy.adi,temp-sense-measurement-interval-ms",
	"debug.ad9361-phy.adi,temp-sense-offset-signed",
	"debug.ad9361-phy.adi,temp-sense-periodic-measurement-enable",
	"debug.ad9361-phy.adi,tx-rf-port-input-select",
	"debug.ad9361-phy.adi,update-tx-gain-in-alert-enable",
	"debug.ad9361-phy.adi,xo-disable-use-ext-refclk-enable",
	"debug.ad9361-phy.adi,fagc-dec-pow-measurement-duration",
	"debug.ad9361-phy.adi,fagc-allow-agc-gain-increase-enable",
	"debug.ad9361-phy.adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt",
	"debug.ad9361-phy.adi,fagc-final-overrange-count",
	"debug.ad9361-phy.adi,fagc-gain-increase-after-gain-lock-enable",
	"debug.ad9361-phy.adi,fagc-gain-index-type-after-exit-rx-mode",
	"debug.ad9361-phy.adi,fagc-lmt-final-settling-steps",
	"debug.ad9361-phy.adi,fagc-lock-level-gain-increase-upper-limit",
	"debug.ad9361-phy.adi,fagc-lock-level-lmt-gain-increase-enable",
	"debug.ad9361-phy.adi,fagc-lp-thresh-increment-steps",
	"debug.ad9361-phy.adi,fagc-lp-thresh-increment-time",
	"debug.ad9361-phy.adi,fagc-lpf-final-settling-steps",
	"debug.ad9361-phy.adi,fagc-optimized-gain-offset",
	"debug.ad9361-phy.adi,fagc-power-measurement-duration-in-state5",
	"debug.ad9361-phy.adi,fagc-rst-gla-en-agc-pulled-high-enable",
	"debug.ad9361-phy.adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable",
	"debug.ad9361-phy.adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll",
	"debug.ad9361-phy.adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable",
	"debug.ad9361-phy.adi,fagc-rst-gla-if-en-agc-pulled-high-mode",
	"debug.ad9361-phy.adi,fagc-rst-gla-large-adc-overload-enable",
	"debug.ad9361-phy.adi,fagc-rst-gla-large-lmt-overload-enable",
	"debug.ad9361-phy.adi,fagc-rst-gla-stronger-sig-thresh-above-ll",
	"debug.ad9361-phy.adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable",
	"debug.ad9361-phy.adi,fagc-state-wait-time-ns",
	"debug.ad9361-phy.adi,fagc-use-last-lock-level-for-set-gain-enable",
	"debug.ad9361-phy.adi,aux-dac-manual-mode-enable",
	"debug.ad9361-phy.adi,aux-dac1-active-in-alert-enable",
	"debug.ad9361-phy.adi,aux-dac1-active-in-rx-enable",
	"debug.ad9361-phy.adi,aux-dac1-active-in-tx-enable",
	"debug.ad9361-phy.adi,aux-dac1-default-value-mV",
	"debug.ad9361-phy.adi,aux-dac1-rx-delay-us",
	"debug.ad9361-phy.adi,aux-dac1-tx-delay-us",
	"debug.ad9361-phy.adi,aux-dac2-active-in-alert-enable",
	"debug.ad9361-phy.adi,aux-dac2-active-in-rx-enable",
	"debug.ad9361-phy.adi,aux-dac2-active-in-tx-enable",
	"debug.ad9361-phy.adi,aux-dac2-default-value-mV",
	"debug.ad9361-phy.adi,aux-dac2-rx-delay-us",
	"debug.ad9361-phy.adi,aux-dac2-tx-delay-us",
	"debug.ad9361-phy.adi,rx-fastlock-delay-ns",
	"debug.ad9361-phy.adi,tx-fastlock-delay-ns",
	"debug.ad9361-phy.adi,rx-fastlock-pincontrol-enable",
	"debug.ad9361-phy.adi,tx-fastlock-pincontrol-enable",
	"debug.ad9361-phy.adi,txmon-1-front-end-gain",
	"debug.ad9361-phy.adi,txmon-1-lo-cm",
	"debug.ad9361-phy.adi,txmon-2-front-end-gain",
	"debug.ad9361-phy.adi,txmon-2-lo-cm",
	"debug.ad9361-phy.adi,txmon-dc-tracking-enable",
	"debug.ad9361-phy.adi,txmon-delay",
	"debug.ad9361-phy.adi,txmon-duration",
	"debug.ad9361-phy.adi,txmon-high-gain",
	"debug.ad9361-phy.adi,txmon-low-gain",
	"debug.ad9361-phy.adi,txmon-low-high-thresh",
	"debug.ad9361-phy.adi,txmon-one-shot-mode-enable",
	SYNC_RELOAD,
	NULL
};

static void update_active_page(gint active_page, gboolean is_detached)
{
	this_page = active_page;
	plugin_detached = is_detached;
}

static void context_destroy(void)
{
	iio_context_destroy(ctx);
}

static bool fmcomms2adv_identify(void)
{
	/* Use the OSC's IIO context just to detect the devices */
	struct iio_context *osc_ctx = get_context_from_osc();
	struct iio_device *osc_dev = iio_context_find_device(
			osc_ctx, "ad9361-phy");
	if (!osc_dev || !iio_device_get_debug_attrs_count(osc_dev))
		return false;

	ctx = osc_create_context();
	dev = iio_context_find_device(ctx, "ad9361-phy");
	dev_slave = iio_context_find_device(ctx, "ad9361-phy-hpc");
	dev_dds_master = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
	if (dev && !iio_device_get_debug_attrs_count(dev))
		dev = NULL;
	if (!dev)
		iio_context_destroy(ctx);

	return !!dev;
}

struct osc_plugin plugin = {
	.name = "FMComms2 Advanced",
	.identify = fmcomms2adv_identify,
	.init = fmcomms2adv_init,
	.save_restore_attribs = fmcomms2_adv_sr_attribs,
	.handle_item = handle_item,
	.update_active_page = update_active_page,
	.destroy = context_destroy,
};
