package com.forklift.controller;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.forklift.controller.logic.CenterMessage;
import com.forklift.controller.logic.CommandChars;
import com.forklift.controller.logic.ControllerState;

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.Iterator;

/**
 * Single screen: connect, pick a load cell and an unload cell, press Start, watch the center's messages.
 * What is highlighted comes only from the center board's replies (ControllerState), never from the taps.
 */
public class MainActivity extends Activity implements BluetoothLink.Listener {
    private static final int REQUEST_BLUETOOTH_CONNECT = 1;
    private static final int MAX_LOG_LINES = 40;

    private final ControllerState state = new ControllerState();
    private final Deque<String> logLines = new ArrayDeque<>();   // oldest first
    private BluetoothLink link;
    private boolean connected = false;

    private CellPanel loadPanel;
    private CellPanel unloadPanel;
    private TextView connectionView;
    private TextView bannerView;
    private TextView jobView;
    private TextView statusView;
    private TextView logView;
    private Button connectButton;
    private Button startButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViews();
        loadPanel = new CellPanel(this, findViewById(R.id.load_cells), CommandChars.LOAD_CELLS, this::sendChar);
        unloadPanel = new CellPanel(this, findViewById(R.id.unload_cells), CommandChars.UNLOAD_CELLS, this::sendChar);
        connectButton.setOnClickListener(view -> onConnectClicked());
        startButton.setOnClickListener(view -> sendChar(CommandChars.START));
        link = new BluetoothLink(this);
        updateUi();
    }

    private void findViews() {
        connectionView = findViewById(R.id.connection_text);
        bannerView = findViewById(R.id.banner_text);
        jobView = findViewById(R.id.job_text);
        statusView = findViewById(R.id.status_text);
        logView = findViewById(R.id.log_text);
        connectButton = findViewById(R.id.connect_button);
        startButton = findViewById(R.id.start_button);
    }

    @Override
    protected void onDestroy() {
        link.shutdown();
        super.onDestroy();
    }

    // ---------------- connect / disconnect ----------------
    private void onConnectClicked() {
        if (connected) {
            link.close();
            connected = false;
            connectionView.setText(R.string.not_connected);
            updateUi();
            return;
        }
        if (!hasConnectPermission()) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, REQUEST_BLUETOOTH_CONNECT);
            return;
        }
        showDevicePicker();
    }

    private boolean hasConnectPermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) return true;   // older Android: normal permission in the manifest
        return checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_BLUETOOTH_CONNECT) return;
        boolean granted = grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
        if (granted) showDevicePicker();
        else Toast.makeText(this, R.string.permission_denied, Toast.LENGTH_LONG).show();
    }

    private void showDevicePicker() {
        BluetoothManager manager = getSystemService(BluetoothManager.class);
        BluetoothAdapter adapter = (manager == null) ? null : manager.getAdapter();
        if (adapter == null) {
            Toast.makeText(this, R.string.bluetooth_unavailable, Toast.LENGTH_LONG).show();
            return;
        }
        if (!adapter.isEnabled()) {
            Toast.makeText(this, R.string.bluetooth_off, Toast.LENGTH_LONG).show();
            return;
        }
        DevicePicker.show(this, adapter, this::startConnecting);
    }

    private void startConnecting(BluetoothDevice device) {
        state.reset();
        logLines.clear();
        connectionView.setText(R.string.connecting);
        updateUi();
        link.connect(device);
    }

    // ---------------- BluetoothLink.Listener (main thread) ----------------
    @Override
    public void onConnected(String deviceName) {
        connected = true;
        connectionView.setText(getString(R.string.connected_format, deviceName));
        updateUi();
    }

    @Override
    public void onLine(String line) {
        addLog("< " + line);
        state.apply(CenterMessage.parse(line));
        updateUi();
    }

    @Override
    public void onClosed(String reason) {
        connected = false;
        connectionView.setText(reason);
        updateUi();
    }

    // ---------------- sending ----------------
    private void sendChar(char c) {
        if (!connected || !CommandChars.isSendable(c)) return;
        link.send(c);
        addLog("> " + c);
        updateUi();
    }

    // ---------------- screen ----------------
    private void addLog(String text) {
        logLines.addLast(text);
        while (logLines.size() > MAX_LOG_LINES) logLines.removeFirst();
    }

    private void updateUi() {
        loadPanel.setSelectedCell(state.getSelectedLoad());
        unloadPanel.setSelectedCell(state.getSelectedUnload());
        loadPanel.setEnabled(connected);
        unloadPanel.setEnabled(connected);
        startButton.setEnabled(connected);
        connectButton.setText(connected ? R.string.disconnect : R.string.connect);
        jobView.setText(state.isJobActive() ? getString(R.string.job_format, state.getJobText()) : getString(R.string.job_none));
        String status = state.getStatusText();
        statusView.setText(status.isEmpty() ? getString(R.string.status_none) : status);
        updateBanner();
        logView.setText(logText());
    }

    private void updateBanner() {
        if (state.isEmergency()) {
            showBanner(R.string.banner_emergency, R.color.banner_emergency);
        } else if (state.isLineLost()) {
            showBanner(R.string.banner_line_lost, R.color.banner_line_lost);
        } else {
            bannerView.setVisibility(View.GONE);
        }
    }

    private void showBanner(int textRes, int colorRes) {
        bannerView.setText(textRes);
        bannerView.setBackgroundColor(getColor(colorRes));
        bannerView.setVisibility(View.VISIBLE);
    }

    /** Newest message first, so no scrolling is needed. */
    private String logText() {
        StringBuilder text = new StringBuilder();
        Iterator<String> newestFirst = logLines.descendingIterator();
        while (newestFirst.hasNext()) text.append(newestFirst.next()).append('\n');
        return text.toString();
    }
}
