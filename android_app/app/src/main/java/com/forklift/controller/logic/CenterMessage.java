package com.forklift.controller.logic;

/**
 * One parsed line from the center board. Texts come from msgText() in master_hw.ino:
 * SEL LOAD x, SEL UNLOAD x, EMPTY x, BUSY x, JOB x>y, JOB RUNNING, ST ..., EMERGENCY, LINE LOST.
 */
public final class CenterMessage {
    public enum Type {
        SELECT_LOAD, SELECT_UNLOAD, EMPTY, BUSY, JOB, JOB_RUNNING, STATE, EMERGENCY, LINE_LOST, UNKNOWN
    }

    private static final String SEL_LOAD = "SEL LOAD ";
    private static final String SEL_UNLOAD = "SEL UNLOAD ";
    private static final String EMPTY_PREFIX = "EMPTY ";
    private static final String BUSY_PREFIX = "BUSY ";
    private static final String JOB_PREFIX = "JOB ";
    private static final String STATE_PREFIX = "ST ";

    public final Type type;
    public final String rawLine;
    /** Load cell '1'..'6' (SELECT_LOAD, EMPTY, JOB), else 0. */
    public final char loadCell;
    /** Unload cell 'A'..'F' (SELECT_UNLOAD, BUSY, JOB), else 0. */
    public final char unloadCell;
    /** Text after "ST " (STATE only), else empty. */
    public final String stateName;

    private CenterMessage(Type type, String rawLine, char loadCell, char unloadCell, String stateName) {
        this.type = type;
        this.rawLine = rawLine;
        this.loadCell = loadCell;
        this.unloadCell = unloadCell;
        this.stateName = stateName;
    }

    /** Never returns null; unrecognized or malformed lines become Type.UNKNOWN. */
    public static CenterMessage parse(String line) {
        String text = (line == null) ? "" : line.trim();
        CenterMessage fixed = parseFixed(text);
        if (fixed != null) return fixed;
        CenterMessage cell = parseCell(text);
        if (cell != null) return cell;
        CenterMessage job = parseJob(text);
        if (job != null) return job;
        if (text.startsWith(STATE_PREFIX) && text.length() > STATE_PREFIX.length()) {
            return new CenterMessage(Type.STATE, text, (char) 0, (char) 0, text.substring(STATE_PREFIX.length()));
        }
        return unknown(text);
    }

    private static CenterMessage parseFixed(String text) {
        if (text.equals("EMERGENCY")) return simple(Type.EMERGENCY, text);
        if (text.equals("LINE LOST")) return simple(Type.LINE_LOST, text);
        if (text.equals("JOB RUNNING")) return simple(Type.JOB_RUNNING, text);
        return null;
    }

    private static CenterMessage parseCell(String text) {
        Character load = oneChar(text, SEL_LOAD);
        if (load != null && CommandChars.isLoadCell(load)) return withLoad(Type.SELECT_LOAD, text, load);
        Character unload = oneChar(text, SEL_UNLOAD);
        if (unload != null && CommandChars.isUnloadCell(unload)) return withUnload(Type.SELECT_UNLOAD, text, unload);
        Character empty = oneChar(text, EMPTY_PREFIX);
        if (empty != null && CommandChars.isLoadCell(empty)) return withLoad(Type.EMPTY, text, empty);
        Character busy = oneChar(text, BUSY_PREFIX);
        if (busy != null && CommandChars.isUnloadCell(busy)) return withUnload(Type.BUSY, text, busy);
        return null;
    }

    /** "JOB 3>B" only. Anything else starting with "JOB " that is not "JOB RUNNING" is unknown. */
    private static CenterMessage parseJob(String text) {
        if (!text.startsWith(JOB_PREFIX) || text.length() != JOB_PREFIX.length() + 3) return null;
        char load = text.charAt(JOB_PREFIX.length());
        char arrow = text.charAt(JOB_PREFIX.length() + 1);
        char unload = text.charAt(JOB_PREFIX.length() + 2);
        if (arrow != '>' || !CommandChars.isLoadCell(load) || !CommandChars.isUnloadCell(unload)) return null;
        return new CenterMessage(Type.JOB, text, load, unload, "");
    }

    /** The single character after prefix, or null if the line is not exactly prefix + 1 character. */
    private static Character oneChar(String text, String prefix) {
        if (!text.startsWith(prefix) || text.length() != prefix.length() + 1) return null;
        return text.charAt(prefix.length());
    }

    private static CenterMessage simple(Type type, String text) {
        return new CenterMessage(type, text, (char) 0, (char) 0, "");
    }

    private static CenterMessage withLoad(Type type, String text, char load) {
        return new CenterMessage(type, text, load, (char) 0, "");
    }

    private static CenterMessage withUnload(Type type, String text, char unload) {
        return new CenterMessage(type, text, (char) 0, unload, "");
    }

    private static CenterMessage unknown(String text) {
        return new CenterMessage(Type.UNKNOWN, text, (char) 0, (char) 0, "");
    }
}
