package com.forklift.controller;

import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/** Dialog listing the phone's already-paired Bluetooth devices. Pairing is done in Android settings. */
final class DevicePicker {
    interface Listener {
        void onPicked(BluetoothDevice device);
    }

    private DevicePicker() { }

    /** The caller must hold BLUETOOTH_CONNECT (Android 12+). */
    static void show(Context context, BluetoothAdapter adapter, Listener listener) {
        final List<BluetoothDevice> devices;
        try {
            devices = new ArrayList<>(adapter.getBondedDevices());
        } catch (SecurityException e) {
            Toast.makeText(context, R.string.permission_denied, Toast.LENGTH_LONG).show();
            return;
        }
        if (devices.isEmpty()) {
            new AlertDialog.Builder(context)
                    .setTitle(R.string.pick_device)
                    .setMessage(R.string.no_paired_devices)
                    .setPositiveButton(android.R.string.ok, null)
                    .show();
            return;
        }
        showList(context, devices, listener);
    }

    private static void showList(Context context, List<BluetoothDevice> devices, Listener listener) {
        String[] labels = new String[devices.size()];
        for (int i = 0; i < labels.length; i++) labels[i] = label(devices.get(i));
        new AlertDialog.Builder(context)
                .setTitle(R.string.pick_device)
                .setItems(labels, (dialog, which) -> listener.onPicked(devices.get(which)))
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    @android.annotation.SuppressLint("MissingPermission")   // show() is only called with the permission granted
    private static String label(BluetoothDevice device) {
        String name = device.getName();
        String shown = (name == null || name.isEmpty()) ? "(no name)" : name;
        return shown + "  " + device.getAddress();
    }
}
