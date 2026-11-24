package com.forklift.controller;

import android.content.Context;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;

/**
 * Six buttons in two rows of three (top row = upper level, bottom row = lower level).
 * The highlighted cell is only changed by the caller (after the center board confirmed the selection).
 */
final class CellPanel {
    interface Listener {
        void onCellClicked(char cell);
    }

    private static final int COLUMNS = 3;
    private static final int ROWS = 2;

    private final String cells;
    private final Button[] buttons;

    CellPanel(Context context, LinearLayout container, String cells, Listener listener) {
        if (cells.length() != COLUMNS * ROWS) {
            throw new IllegalArgumentException("cells must have " + (COLUMNS * ROWS) + " characters: " + cells);
        }
        this.cells = cells;
        this.buttons = new Button[cells.length()];
        for (int row = 0; row < ROWS; row++) {
            container.addView(buildRow(context, row, listener));
        }
    }

    private LinearLayout buildRow(Context context, int row, Listener listener) {
        LinearLayout rowLayout = new LinearLayout(context);
        rowLayout.setOrientation(LinearLayout.HORIZONTAL);
        for (int column = 0; column < COLUMNS; column++) {
            int index = row * COLUMNS + column;
            Button button = buildButton(context, cells.charAt(index), listener);
            buttons[index] = button;
            rowLayout.addView(button);
        }
        return rowLayout;
    }

    private Button buildButton(Context context, char cell, Listener listener) {
        Button button = new Button(context);
        button.setText(String.valueOf(cell));
        button.setBackgroundResource(R.drawable.cell_button);
        int margin = context.getResources().getDimensionPixelSize(R.dimen.cell_margin);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                0, context.getResources().getDimensionPixelSize(R.dimen.cell_height), 1f);
        params.setMargins(margin, margin, margin, margin);
        button.setLayoutParams(params);
        button.setOnClickListener((View view) -> listener.onCellClicked(cell));
        return button;
    }

    /** cell = one of this panel's characters, or 0 for "none". */
    void setSelectedCell(char cell) {
        for (int i = 0; i < buttons.length; i++) {
            buttons[i].setSelected(cells.charAt(i) == cell);
        }
    }

    void setEnabled(boolean enabled) {
        for (Button button : buttons) button.setEnabled(enabled);
    }
}
