package com.microslots;

import android.content.Context;
import android.webkit.WebView;
import android.webkit.WebChromeClient;
import android.webkit.WebViewClient;
import android.webkit.WebSettings;
import android.widget.AbsoluteLayout;
import android.util.Log;

import com.microslots.global;

// webView.setWebViewClient(new WebViewClient()
public class myWebView extends WebView 
{
	public myWebView(int w, int h, int x, int y)
	{
		super(global.s_context);

		setWebViewClient(new myWebClient());
		setWebChromeClient(new WebChromeClient());

		setBackgroundColor(0);	// transparent
		clearCache(true);

		WebSettings webSettings = getSettings();
		webSettings.setJavaScriptEnabled(true);
		webSettings.setAppCacheEnabled(false);
		webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);

		 AbsoluteLayout.LayoutParams lp = new AbsoluteLayout.LayoutParams(w, h, x, y); 
		 global.s_ll.addView(this, lp);
	}
	
	public void remove()
	{
		 global.s_ll.removeView(this);
	}

	// 0 = visible
	// 4 = invisible
	// 8 = gone
	public void setVisible(int visible)
	{
		setVisibility(visible);

		WebSettings webSettings = getSettings();
		webSettings.setJavaScriptEnabled(visible == 0 ? true : false);
	}

	@Override public void loadUrl (String url)
	{
		super.loadUrl(url);
		com.microslots.bakeinflash.sendMessage("onLoadURL", url);
	}

	private class myWebClient extends WebViewClient
	{
		@Override public boolean shouldOverrideUrlLoading(WebView view, String url)  
		{  
			if (url.startsWith("callback:"))  
			{  
				// Parse further to extract function and do custom action  
				com.microslots.bakeinflash.sendMessage("onCallback", url);
				return true;
			}
			// Load the page via the webview
			com.microslots.bakeinflash.sendMessage("onLoadURL", url);
			view.loadUrl(url);  
			return true;  
		} 

         @Override public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) 
		 {
			Log.e("BakeInFlash", description + ": " + failingUrl);	  
			com.microslots.bakeinflash.sendMessage("onLoadURLError", description + ": " + failingUrl);
		}
	}
}