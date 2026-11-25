package com.forklift.controller.logic;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class CenterMessageTest {
    private static CenterMessage.Type typeOf(String line) {
        return CenterMessage.parse(line).type;
    }

    @Test
    public void selectionLines() {
        CenterMessage load = CenterMessage.parse("SEL LOAD 3");
        assertEquals(CenterMessage.Type.SELECT_LOAD, load.type);
        assertEquals('3', load.loadCell);
        CenterMessage unload = CenterMessage.parse("SEL UNLOAD B");
        assertEquals(CenterMessage.Type.SELECT_UNLOAD, unload.type);
        assertEquals('B', unload.unloadCell);
    }

    @Test
    public void rejectionLines() {
        assertEquals(CenterMessage.Type.EMPTY, typeOf("EMPTY 3"));
        assertEquals('3', CenterMessage.parse("EMPTY 3").loadCell);
        assertEquals(CenterMessage.Type.BUSY, typeOf("BUSY B"));
        assertEquals('B', CenterMessage.parse("BUSY B").unloadCell);
        assertEquals(CenterMessage.Type.JOB_RUNNING, typeOf("JOB RUNNING"));
    }

    @Test
    public void jobLine() {
        CenterMessage job = CenterMessage.parse("JOB 3>B");
        assertEquals(CenterMessage.Type.JOB, job.type);
        assertEquals('3', job.loadCell);
        assertEquals('B', job.unloadCell);
    }

    @Test
    public void stateAndAlarmLines() {
        CenterMessage state = CenterMessage.parse("ST TO LOAD");
        assertEquals(CenterMessage.Type.STATE, state.type);
        assertEquals("TO LOAD", state.stateName);
        assertEquals(CenterMessage.Type.EMERGENCY, typeOf("EMERGENCY"));
        assertEquals(CenterMessage.Type.LINE_LOST, typeOf("LINE LOST"));
    }

    @Test
    public void malformedLinesAreUnknown() {
        String[] bad = {"", "SEL LOAD", "SEL LOAD 7", "SEL LOAD A", "SEL UNLOAD 3", "SEL LOAD 33",
                        "EMPTY B", "BUSY 3", "JOB 3B", "JOB 7>B", "JOB 3>G", "ST ", "ST", "hello"};
        for (String line : bad) {
            assertEquals(line, CenterMessage.Type.UNKNOWN, typeOf(line));
        }
    }

    @Test
    public void nullAndWhitespaceAreHandled() {
        assertEquals(CenterMessage.Type.UNKNOWN, typeOf(null));
        assertEquals(CenterMessage.Type.JOB, typeOf("  JOB 3>B  "));
    }
}
