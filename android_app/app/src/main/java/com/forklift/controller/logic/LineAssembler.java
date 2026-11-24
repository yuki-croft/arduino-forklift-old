package com.forklift.controller.logic;

import java.util.ArrayList;
import java.util.List;

/**
 * Turns the raw byte stream from the center board into text lines.
 * The center ends every message with '\n'. '\r' and non-printable bytes are ignored.
 * A line longer than MAX_LINE_LENGTH is dropped completely (up to its '\n').
 */
public final class LineAssembler {
    public static final int MAX_LINE_LENGTH = 32;

    private final StringBuilder buffer = new StringBuilder();
    private boolean overflowing = false;

    /** Feeds data[0..length) and returns the lines completed by it (never null). */
    public List<String> feed(byte[] data, int length) {
        List<String> lines = new ArrayList<>();
        int count = Math.min(length, data.length);
        for (int i = 0; i < count; i++) {
            String line = feedByte(data[i]);
            if (line != null) lines.add(line);
        }
        return lines;
    }

    private String feedByte(byte value) {
        char c = (char) (value & 0xFF);
        if (c == '\n') return endLine();
        if (c < 0x20 || c > 0x7E) return null;   // also skips '\r'
        if (overflowing) return null;
        if (buffer.length() >= MAX_LINE_LENGTH) {
            overflowing = true;
            buffer.setLength(0);
            return null;
        }
        buffer.append(c);
        return null;
    }

    private String endLine() {
        String line = buffer.toString().trim();
        buffer.setLength(0);
        boolean wasOverflowing = overflowing;
        overflowing = false;
        if (wasOverflowing || line.isEmpty()) return null;
        return line;
    }

    /** Forget any partial line (call when a new connection starts). */
    public void reset() {
        buffer.setLength(0);
        overflowing = false;
    }
}
