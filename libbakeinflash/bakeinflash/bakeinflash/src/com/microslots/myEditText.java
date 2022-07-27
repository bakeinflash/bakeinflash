package com.microslots;

import android.content.Context;
import android.widget.AbsoluteLayout;
import android.graphics.Typeface;
import android.widget.EditText;
import android.view.inputmethod.EditorInfo;

import com.microslots.global;

public class myEditText extends EditText 
{
	public myEditText(int w, int h, int x, int y, int fontsize, int textColor, int bgColor, boolean enabled, int align, String font, int style)
	{
		super(global.s_context);
		
		setPadding(0, 0, 0, 0);
		float den = global.s_javaDen;
		setTextSize(fontsize / den);
		setHeight(h);
		setWidth(w);
		setTextColor(textColor);
		setBackgroundColor(bgColor);
		setEnabled(enabled);
		setGravity(align);

	//	setInputType(TYPE_CLASS_TEXT);
		setSingleLine();
		setImeActionLabel("Done", EditorInfo.IME_ACTION_DONE);
		setImeOptions(EditorInfo.IME_ACTION_DONE);

		Typeface typeface = Typeface.create(font, style);
		setTypeface(typeface);

		AbsoluteLayout.LayoutParams lp = new AbsoluteLayout.LayoutParams(w, h, x, y); 
		global.s_ll.addView(this, lp);
	}

	public String getValue(int visible)
	{
		setVisibility(visible);
		return getText().toString();
	}

	public void remove()
	{
		 global.s_ll.removeView(this);
	}

}