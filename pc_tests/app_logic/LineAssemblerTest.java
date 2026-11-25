package com.forklift.controller.logic;

import static org.junit.Assert.assertEquals;

import java.util.List;
import org.junit.Test;

public class LineAssemblerTest {
    private static List<String> feed(LineAssembler a, String s) {
        byte[] b = s.getBytes(java.nio.charset.StandardCharsets.ISO_8859_1);
        return a.feed(b, b.length);
    }

    @Test
    public void completeLineIsReturned() {
        assertEquals("[SEL LOAD 3]", feed(new LineAssembler(), "SEL LOAD 3\n").toString());
    }

    @Test
    public void lineSplitAcrossReadsIsJoined() {
        LineAssembler a = new LineAssembler();
        assertEquals(0, feed(a, "ST TO ").size());
        assertEquals("[ST TO LOAD]", feed(a, "LOAD\n").toString());
    }

    @Test
    public void carriageReturnAndEmptyLinesAreIgnored() {
        assertEquals("[EMERGENCY]", feed(new LineAssembler(), "\r\n\nEMERGENCY\r\n").toString());
    }

    @Test
    public void severalLinesInOneRead() {
        assertEquals("[ST WAIT, JOB 3>B]", feed(new LineAssembler(), "ST WAIT\nJOB 3>B\n").toString());
    }

    @Test
    public void nonPrintableBytesAreDropped() {
        assertEquals("[ST DONE]", feed(new LineAssembler(), "ST\u0001 DOÿNE\n").toString());
    }

    @Test
    public void tooLongLineIsDroppedWholeAndNextLineSurvives() {
        StringBuilder longLine = new StringBuilder();
        for (int i = 0; i < LineAssembler.MAX_LINE_LENGTH + 5; i++) longLine.append('X');
        LineAssembler a = new LineAssembler();
        assertEquals("[]", feed(a, longLine + "\n").toString());
        assertEquals("[ST WAIT]", feed(a, "ST WAIT\n").toString());
    }

    @Test
    public void lineOfExactlyMaxLengthIsKept() {
        StringBuilder maxLine = new StringBuilder();
        for (int i = 0; i < LineAssembler.MAX_LINE_LENGTH; i++) maxLine.append('Y');
        assertEquals(1, feed(new LineAssembler(), maxLine + "\n").size());
    }

    @Test
    public void resetForgetsPartialLine() {
        LineAssembler a = new LineAssembler();
        feed(a, "ST TO");
        a.reset();
        assertEquals("[ST WAIT]", feed(a, "ST WAIT\n").toString());
    }

    @Test
    public void lengthSmallerThanArrayOnlyUsesLength() {
        byte[] b = "ST WAIT\nGARBAGE\n".getBytes(java.nio.charset.StandardCharsets.ISO_8859_1);
        assertEquals("[ST WAIT]", new LineAssembler().feed(b, 8).toString());
    }
}
