package com.forklift.controller.logic;

/**
 * What the screen shows, derived only from lines the center board sent.
 * The center decides everything (occupancy, rejection); the app just mirrors it.
 */
public final class ControllerState {
    private static final String STATE_WAIT = "WAIT";
    private static final String STATE_DONE = "DONE";

    private char selectedLoad = 0;
    private char selectedUnload = 0;
    private String jobText = "";
    private String statusText = "";
    private boolean emergency = false;
    private boolean lineLost = false;

    public char getSelectedLoad() { return selectedLoad; }
    public char getSelectedUnload() { return selectedUnload; }
    /** "3>B" while a job runs, otherwise "". */
    public String getJobText() { return jobText; }
    /** Last status line from the center, e.g. "ST TO LOAD" or "BUSY B". */
    public String getStatusText() { return statusText; }
    public boolean isEmergency() { return emergency; }
    public boolean isLineLost() { return lineLost; }
    public boolean isJobActive() { return !jobText.isEmpty(); }

    /** New connection: nothing is known about the center yet. */
    public void reset() {
        selectedLoad = 0;
        selectedUnload = 0;
        jobText = "";
        statusText = "";
        emergency = false;
        lineLost = false;
    }

    public void apply(CenterMessage message) {
        switch (message.type) {
            case SELECT_LOAD:
                selectedLoad = message.loadCell;
                break;
            case SELECT_UNLOAD:
                selectedUnload = message.unloadCell;
                break;
            case JOB:
                jobText = message.loadCell + ">" + message.unloadCell;
                break;
            case EMERGENCY:
                emergency = true;
                break;
            case LINE_LOST:
                lineLost = true;
                break;
            case STATE:
                applyState(message.stateName);
                break;
            default:
                break;   // EMPTY, BUSY, JOB_RUNNING, UNKNOWN: only the status line changes
        }
        statusText = message.rawLine;
    }

    /** A new state ends "line lost"; arriving in WAIT or DONE ends the job (and any emergency). */
    private void applyState(String stateName) {
        lineLost = false;
        if (!stateName.equals(STATE_WAIT) && !stateName.equals(STATE_DONE)) return;
        emergency = false;
        if (isJobActive()) {
            selectedLoad = 0;
            selectedUnload = 0;
            jobText = "";
        }
    }
}
