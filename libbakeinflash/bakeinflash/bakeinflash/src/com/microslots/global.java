package com.microslots;

import android.content.Context;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebSettings;
import android.widget.AbsoluteLayout;

// webView.setWebViewClient(new WebViewClient()

public class global
{
	public static Context s_context;
	public static AbsoluteLayout s_ll;
	public static float s_javaDen;
	public static int s_API;

	public static int getAPI()
	{
		return s_API;
	}
}