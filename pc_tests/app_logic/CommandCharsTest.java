package com.forklift.controller.logic;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class CommandCharsTest {
    @Test
    public void allTwelveCellsAndStartAreSendable() {
        for (char c : "123456ABCDEFS".toCharArray()) assertTrue("" + c, CommandChars.isSendable(c));
    }

    @Test
    public void everythingElseIsNotSendable() {
        for (char c : "07GXa\n \u0000s".toCharArray()) assertFalse("" + (int) c, CommandChars.isSendable(c));
    }

    @Test
    public void loadAndUnloadAreDistinct() {
        assertTrue(CommandChars.isLoadCell('1'));
        assertFalse(CommandChars.isLoadCell('A'));
        assertTrue(CommandChars.isUnloadCell('F'));
        assertFalse(CommandChars.isUnloadCell('6'));
    }
}
