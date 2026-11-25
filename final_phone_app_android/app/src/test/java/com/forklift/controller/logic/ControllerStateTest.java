package com.forklift.controller.logic;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class ControllerStateTest {
    private static void send(ControllerState s, String... lines) {
        for (String line : lines) s.apply(CenterMessage.parse(line));
    }

    @Test
    public void selectionIsShownOnlyAfterCenterConfirms() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 3", "SEL UNLOAD B");
        assertEquals('3', s.getSelectedLoad());
        assertEquals('B', s.getSelectedUnload());
    }

    @Test
    public void rejectionKeepsSelectionAndShowsReason() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 3", "EMPTY 4");
        assertEquals('3', s.getSelectedLoad());
        assertEquals("EMPTY 4", s.getStatusText());
        send(s, "BUSY C");
        assertEquals("BUSY C", s.getStatusText());
        send(s, "JOB RUNNING");
        assertEquals("JOB RUNNING", s.getStatusText());
        assertEquals('3', s.getSelectedLoad());
    }

    @Test
    public void jobStartAndNormalFinish() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 3", "SEL UNLOAD B", "JOB 3>B", "ST TO LOAD");
        assertTrue(s.isJobActive());
        assertEquals("3>B", s.getJobText());
        assertEquals("ST TO LOAD", s.getStatusText());
        assertEquals('3', s.getSelectedLoad());
        send(s, "ST DONE");
        assertFalse(s.isJobActive());
        assertEquals(0, s.getSelectedLoad());
        assertEquals(0, s.getSelectedUnload());
        assertEquals("ST DONE", s.getStatusText());
    }

    @Test
    public void emergencyThenArrivalClearsEverything() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 1", "SEL UNLOAD A", "JOB 1>A", "EMERGENCY");
        assertTrue(s.isEmergency());
        assertEquals("EMERGENCY", s.getStatusText());
        send(s, "ST WAIT");
        assertFalse(s.isEmergency());
        assertFalse(s.isJobActive());
        assertEquals(0, s.getSelectedLoad());
    }

    @Test
    public void lineLostEndsWithNextState() {
        ControllerState s = new ControllerState();
        send(s, "JOB 1>A", "LINE LOST");
        assertTrue(s.isLineLost());
        send(s, "ST APPROACH");
        assertFalse(s.isLineLost());
    }

    @Test
    public void waitWithoutActiveJobKeepsUserSelection() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 2", "ST WAIT");   // e.g. the forklift was rebooted before the start button
        assertEquals('2', s.getSelectedLoad());
    }

    @Test
    public void unknownLineOnlyChangesStatus() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 2", "garbage");
        assertEquals('2', s.getSelectedLoad());
        assertEquals("garbage", s.getStatusText());
    }

    @Test
    public void resetForgetsEverything() {
        ControllerState s = new ControllerState();
        send(s, "SEL LOAD 2", "JOB 2>A", "EMERGENCY", "LINE LOST");
        s.reset();
        assertEquals(0, s.getSelectedLoad());
        assertFalse(s.isJobActive());
        assertFalse(s.isEmergency());
        assertFalse(s.isLineLost());
        assertEquals("", s.getStatusText());
    }
}
