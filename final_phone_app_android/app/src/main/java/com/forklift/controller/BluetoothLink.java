package com.forklift.controller;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Looper;

import com.forklift.controller.logic.LineAssembler;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Classic Bluetooth (SPP / RFCOMM) link to the center board's phone module.
 * All network I/O runs on background threads; every Listener callback runs on the main thread.
 * connect(), send() and close() must be called from the main thread.
 * MainActivity checks the BLUETOOTH_CONNECT permission before it calls connect().
 */
@SuppressLint("MissingPermission")
final class BluetoothLink {
    interface Listener {
        void onConnected(String deviceName);
        void onLine(String line);
        void onClosed(String reason);
    }

    /** Standard Serial Port Profile UUID (used by HC-05 / HC-06). */
    static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int READ_BUFFER_BYTES = 64;

    private final Listener listener;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final ExecutorService writer = Executors.newSingleThreadExecutor();
    private volatile BluetoothSocket socket;
    /** Bumped by connect() and close() (main thread only); callbacks of an older generation are dropped. */
    private volatile int generation = 0;

    BluetoothLink(Listener listener) {
        this.listener = listener;
    }

    void connect(BluetoothDevice device) {
        close();
        final int myGeneration = generation;
        Thread thread = new Thread(() -> runConnection(device, myGeneration), "bt-link");
        thread.start();
    }

    /** Sends one character. The caller must have validated it (CommandChars.isSendable). */
    void send(char c) {
        final BluetoothSocket target = socket;
        if (target == null) return;
        final int myGeneration = generation;
        writer.execute(() -> writeByte(target, c, myGeneration));
    }

    /** Closes the current link, if any. No callback is delivered for a close requested here. */
    void close() {
        generation++;
        BluetoothSocket old = socket;
        socket = null;
        closeQuietly(old);
    }

    void shutdown() {
        close();
        writer.shutdownNow();
    }

    // ---------------- background threads ----------------
    private void runConnection(BluetoothDevice device, int myGeneration) {
        BluetoothSocket created = null;
        try {
            created = device.createRfcommSocketToServiceRecord(SPP_UUID);
            socket = created;
            created.connect();
            final String label = deviceLabel(device);
            post(myGeneration, () -> listener.onConnected(label));
            readLoop(created, myGeneration);
            post(myGeneration, () -> listener.onClosed("Connection closed"));
        } catch (IOException e) {
            final String reason = "Connection failed: " + e.getMessage();
            post(myGeneration, () -> listener.onClosed(reason));
        } catch (SecurityException e) {
            post(myGeneration, () -> listener.onClosed("Bluetooth permission missing"));
        } finally {
            if (socket == created) socket = null;
            closeQuietly(created);
        }
    }

    private void readLoop(BluetoothSocket connected, int myGeneration) throws IOException {
        InputStream in = connected.getInputStream();
        byte[] buffer = new byte[READ_BUFFER_BYTES];
        LineAssembler assembler = new LineAssembler();
        int count;
        while ((count = in.read(buffer)) != -1) {
            for (String line : assembler.feed(buffer, count)) {
                final String text = line;
                post(myGeneration, () -> listener.onLine(text));
            }
        }
    }

    private void writeByte(BluetoothSocket target, char c, int myGeneration) {
        try {
            OutputStream out = target.getOutputStream();
            out.write((byte) c);
            out.flush();
        } catch (IOException e) {
            final String reason = "Send failed: " + e.getMessage();
            post(myGeneration, () -> listener.onClosed(reason));
        }
    }

    // ---------------- helpers ----------------
    private void post(int myGeneration, Runnable action) {
        mainHandler.post(() -> {
            if (myGeneration == generation) action.run();
        });
    }

    private static String deviceLabel(BluetoothDevice device) {
        try {
            String name = device.getName();
            return (name == null || name.isEmpty()) ? device.getAddress() : name;
        } catch (SecurityException e) {
            return device.getAddress();
        }
    }

    private static void closeQuietly(BluetoothSocket target) {
        if (target == null) return;
        try {
            target.close();
        } catch (IOException ignored) {
            // closing a socket that is already broken: nothing more to do
        }
    }
}
