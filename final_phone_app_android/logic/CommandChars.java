package com.forklift.controller.logic;

/**
 * Characters the phone may send to the center board (PROBLEM_1PAGER.md, "phone -> center").
 * Pure Java: no Android classes, so it is compiled and tested on a PC.
 */
public final class CommandChars {
    /** TODO: must match START_CHAR in final_center_board/final_center_board.ino (temporary value). */
    public static final char START = 'S';

    public static final String LOAD_CELLS = "123456";    // 1-3 = upper level, 4-6 = lower level
    public static final String UNLOAD_CELLS = "ABCDEF";  // A-C = upper level, D-F = lower level

    private CommandChars() { }

    public static boolean isLoadCell(char c) {
        return LOAD_CELLS.indexOf(c) >= 0;
    }

    public static boolean isUnloadCell(char c) {
        return UNLOAD_CELLS.indexOf(c) >= 0;
    }

    /** Only these characters may ever be written to the Bluetooth link. */
    public static boolean isSendable(char c) {
        return c == START || isLoadCell(c) || isUnloadCell(c);
    }
}
