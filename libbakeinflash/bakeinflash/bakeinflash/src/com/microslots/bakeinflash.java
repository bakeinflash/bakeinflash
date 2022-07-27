//
//	BakeInFlash, 2011
//
package com.microslots;

import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.File;
import java.io.OutputStream;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;
import android.app.Activity;
import android.content.Context;
//import android.content.SharedPreferences;
import android.os.Build; //.VERSION.SDK_INT;
import android.content.pm.ActivityInfo;
import android.content.res.AssetFileDescriptor;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.SurfaceHolder.Callback;
import android.widget.AbsoluteLayout;
import android.view.KeyEvent;

import android.app.Application;
import android.content.Context;
import android.content.res.AssetManager;
//import android.preference.PreferenceManager;

// bakeinflash java classes
import com.microslots.myWebView;
import com.microslots.global;

// for isinstalled
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;


//@SuppressWarnings("deprecation")
public class bakeinflash extends Activity	// implements SensorEventListener 
{
	private Activity mActivity;		// this

	static
	{
//		android.os.Debug.waitForDebugger();
//		System.loadLibrary("ngtserver");
		System.loadLibrary("bakeinflash");
	}
	public static native void sendMessage(String name, String arg);
	private static native void render();
	private static native void onKeyEvent(int keycode, boolean state);
	private static native void notifyMouseState(int x, int y, int state);
	private static native void loadSWF(String pkgName, String appPath, boolean hasStencil);
	private static native void pauseSWF();
	private static native void resumeSWF();
	private static native void setView(int w, int h);
  	
	@Override public boolean onKeyUp(int keyCode, KeyEvent event)
	{
      //  Log.e("java BakeInFlash", "onKeyUp "+keyCode);	  
        onKeyEvent(keyCode, false);
        return super.onKeyUp(keyCode, event);
		
/*	    switch (keyCode)
	    {
	        case KeyEvent.KEYCODE_D:
//	            moveShip(MOVE_LEFT);
	            return true;
	        case KeyEvent.KEYCODE_F:
	//            moveShip(MOVE_RIGHT);
	            return true;
	        case KeyEvent.KEYCODE_J:
	  //          fireMachineGun();
	            return true;
	        case KeyEvent.KEYCODE_K:
	    //        fireMissile();
	            return true;
	        default:
	            return super.onKeyUp(keyCode, event);
	    }*/
	}
	
	@Override public boolean onKeyDown(int keyCode, KeyEvent event) 
	{
      //  Log.e("java BakeInFlash", "onKeyDown "+keyCode);
        onKeyEvent(keyCode, true);
        return super.onKeyDown(keyCode, event);
/*        
	    switch (keyCode) 
	    {
	        case KeyEvent.KEYCODE_J:
	            if (event.isShiftPressed()) {
	      //          fireLaser();
	            } else {
	        //        fireMachineGun();
	            }
	            return true;
	        case KeyEvent.KEYCODE_K:
	            if (event.isShiftPressed()) {
	          //      fireSeekingMissle();
	            } else {
	            //    fireMissile();
	            }
	            return true;
	        default:
	            return super.onKeyUp(keyCode, event);
	    }
	    */
	}
	
	public boolean onTouchEvent(final MotionEvent event)
	{
		if (event.getAction() == MotionEvent.ACTION_DOWN) 
		{
			notifyMouseState((int) event.getX(), (int) event.getY(), 0);
			render();
			notifyMouseState((int) event.getX(), (int) event.getY(), 1);
			render();
		}
		else
		if (event.getAction() == MotionEvent.ACTION_UP) 
		{
			notifyMouseState((int) event.getX(), (int) event.getY(), 1);
			render();
			notifyMouseState((int) event.getX(), (int) event.getY(), 0);
			render();
		}
		else
		if (event.getAction() == MotionEvent.ACTION_MOVE) 
		{
			notifyMouseState((int) event.getX(), (int) event.getY(), 1);
			render();
		}
		return true;
	}
  	
	@Override protected void onResume()
    {
        Log.e("BakeInFlash", "Java: onResume");	  
		//requestRandomStickeePosition();
        super.onResume();
        
//        mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_NORMAL);
        
    	resumeSWF();
    }

	@Override protected void onPause()
    {
        Log.e("BakeInFlash", "Java: onPause");	  
        super.onResume();
 
//        mSensorManager.unregisterListener(this);
        
    	pauseSWF();
    }

	@Override protected void onStart()
    {
        Log.e("BakeInFlash", "Java: onStart");	  
        super.onStart();
    }

	@Override protected void onRestart()
    {
        Log.e("BakeInFlash", "Java: onRestart");	  
        super.onRestart();
    }
	
	@Override protected void onStop()
    {
        Log.e("BakeInFlash", "Java: onStop");	  
        super.onStop();
    }
	
	@Override protected void onDestroy()
    {
        Log.e("BakeInFlash", "Java: onDestroy");	  
        super.onDestroy();
    }
	
	public static long getFreeMem()
	{
		long freeSize = 0L;
		long totalSize = 0L;
		long usedSize = -1L;
		try {
			Runtime info = Runtime.getRuntime();
			freeSize = info.freeMemory();
			totalSize = info.totalMemory();
			usedSize = totalSize - freeSize;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return freeSize;
	}

    private static boolean copyAssetFolder(AssetManager assetManager, String fromAssetPath, String toPath) 
    {
        try 
        {
            String[] files = assetManager.list(fromAssetPath);
            new File(toPath).mkdirs();
            boolean res = true;
            for (String file : files)
            {
                String fromPath = fromAssetPath == "" ? file : fromAssetPath + "/" + file;
                if (file.contains("."))
                {
                	res &= copyAsset(assetManager, fromPath, toPath + "/" + file);
                }
                else
                {
                	res &= copyAssetFolder(assetManager, fromPath, toPath + "/" + file);
                }
            }
            return res;
        }
        catch (Exception e) 
        {
            e.printStackTrace();
            return false;
        }
    }

    private static boolean copyAsset(AssetManager assetManager, String fromAssetPath, String toPath)
    {
    	Log.e("bakeinflash",  "copy asset file from '" + fromAssetPath + "' to '" + toPath + "'");
        	  
        InputStream in = null;
        OutputStream out = null;
        try
        {
          in = assetManager.open(fromAssetPath);
          new File(toPath).createNewFile();
          out = new FileOutputStream(toPath);
          copyFile(in, out);
          in.close();
          in = null;
          out.flush();
          out.close();
          out = null;
          return true;
        }
        catch(Exception e)
        {
        	Log.e("bakeinflash", "Could'n coppy asset file " + fromAssetPath);
            e.printStackTrace();
            return false;
        }
    }

    private static void copyFile(InputStream in, OutputStream out) throws IOException 
    {
        byte[] buffer = new byte[1024];
        int read;
        int total = 0;
        while((read = in.read(buffer)) != -1)
        {
          out.write(buffer, 0, read);
          total += read;
        }
        Log.e("bakeinflash", "copied " + total + " bytes");
    }
	
	@Override  protected void onCreate(Bundle saved) 
    {
	  Log.e("BakeInFlash", "Java: onCreate");	  
	  mActivity = this;

      super.onCreate(saved);
    //  setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		 
//      mSensorManager = (SensorManager)getSystemService(SENSOR_SERVICE);
//      mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

	int ver = Build.VERSION.SDK_INT;
	if (ver < 19) 
	{
      requestWindowFeature(Window.FEATURE_NO_TITLE);
      getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
	}

      final EGL10 egl = (EGL10) EGLContext.getEGL();
      final EGLDisplay eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
      int[] version = new int[2];
      egl.eglInitialize(eglDisplay, version);

//	  setEGLConfigChooser(8, 8, 8, 8, 16, 8);
      int[] configSpecMin = 
      {
          EGL10.EGL_NONE
      };
      int[] configSpecStencil = 
      {
	  	  EGL10.EGL_RED_SIZE, 8,
    	  EGL10.EGL_GREEN_SIZE, 8,
   		  EGL10.EGL_BLUE_SIZE, 8,
         // EGL10.EGL_DEPTH_SIZE, 16,
          EGL10.EGL_STENCIL_SIZE, 8,
          EGL10.EGL_NONE
      };
      int[] configSpecDepth = 
      {
	  	  EGL10.EGL_RED_SIZE, 8,
    	  EGL10.EGL_GREEN_SIZE, 8,
   		  EGL10.EGL_BLUE_SIZE, 8,
          EGL10.EGL_DEPTH_SIZE, 16,
         // EGL10.EGL_STENCIL_SIZE, 8,
          EGL10.EGL_NONE
      };

      final EGLConfig[] config = new EGLConfig[1];
      int num_configs[] = new int[1];
	  boolean hasStencil = true;
	  egl.eglChooseConfig(eglDisplay, configSpecStencil, config, 1, num_configs);
	  if (config[0] == null)
	  {
		hasStencil = false;
	    egl.eglChooseConfig(eglDisplay, configSpecDepth, config, 1, num_configs);
	  }
	  if (config[0] == null)
	  {
		hasStencil = false;
	    egl.eglChooseConfig(eglDisplay, configSpecMin, config, 1, num_configs);
	  }

      final EGLContext eglContext = egl.eglCreateContext(eglDisplay, config[0], EGL10.EGL_NO_CONTEXT, null);

      // Setting up layouts and views
      SurfaceView view = new SurfaceView(this);
      setContentView(view);

      global.s_context = getApplicationContext();
      global.s_javaDen = getApplication().getResources().getDisplayMetrics().density;
      global.s_API = android.os.Build.VERSION.SDK_INT;
      global.s_ll = new AbsoluteLayout(this);

      addContentView(global.s_ll, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
     
	if (ver >= 19) 
	{
        View decorView = getWindow().getDecorView();
	    int uiOptions = 0;
		uiOptions |= 4; //View.SYSTEM_UI_FLAG_FULLSCREEN;
		uiOptions |= 2; //View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
		uiOptions |= 2048; //View.SYSTEM_UI_FLAG_IMMERSIVE;
		decorView.setSystemUiVisibility(uiOptions);

		getActionBar().hide();
    }


//      SharedPreferences pref = getPreferences(MODE_PRIVATE);
  	  String appPath = global.s_context.getFilesDir().getAbsolutePath(); 
  	  appPath += "/my";
      Log.e("bakeinflash", appPath);
      	  
   	  // copy asset
      String path = "/data/data/com.microslots/files/my";
      boolean installed = (new File(path)).exists(); 
//	  if (!installed)
	  if (true)
	  {
   		  copyAssetFolder(getAssets(), "my", path);
   	  }
   	  else
   	  {
          Log.e("BakeInFlash", "assets are not copied..used old files");	  
   	  }
   	  
	  SurfaceHolder holder = view.getHolder();
      holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
      final GL10 gl = (GL10) eglContext.getGL();
      final Handler handler = new Handler();

   	  loadSWF("com/microslots/", appPath, hasStencil);

	  	// admob
		/*
		String admob_hash = getString(R.string.admob);
		if (admob_hash.length() > 0)
		{
			// fixme: android:configChanges="keyboardHidden|orientation|screenSize"
			MobileCore.init(this, admob_hash, LOG_TYPE.DEBUG, AD_UNITS.STICKEEZ,AD_UNITS.INTERSTITIAL,AD_UNITS.DIRECT_TO_MARKET,AD_UNITS.NATIVE_ADS);
			MobileCore.setNativeAdsBannerSupport(true);
			setAdUnitsEventListener();
		}
		*/
        
      holder.addCallback(new Callback() 
      {
        private EGLSurface surface;
        private Runnable painter;

        @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) 
        {
        	setView(width, height);
        }
        
        @Override  public void surfaceCreated(SurfaceHolder holder)
        {
          surface = egl.eglCreateWindowSurface(eglDisplay, config[0], holder, null);
          egl.eglMakeCurrent(eglDisplay, surface, surface, eglContext);
          painter = new Runnable()
          {
        	  @Override public void run()
        	  {
        		  drawFrame(gl);
	              egl.eglSwapBuffers(eglDisplay, surface);
	              handler.post(this);
        	  }
          };
          handler.post(painter);
        }
        
        @Override public void surfaceDestroyed(SurfaceHolder holder)
        {
          handler.removeCallbacks(painter);
        }
        
      }
    );
      
  }

	private boolean isAppInstalled(String uri)
	{
		PackageManager pm = getPackageManager();
		boolean installed = false;
		try
		{
			pm.getPackageInfo(uri, PackageManager.GET_ACTIVITIES);
			installed = true;
		}
		catch (PackageManager.NameNotFoundException e)
		{
			installed = false;
		}
		return installed;
	}

	// system FPS=60
  private void drawFrame(GL10 gl)
  {
	  gl.glClearColor(0.f, 0.f, 0.f, 0.f);
	  gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
	  render();
	//  Log.e("java BakeInFlash", "freemem: " + getFreeMem() / 1024 + "kB");	  
  }

  // admod
  /*
	private void setAdUnitsEventListener()
	{
		MobileCore.setAdUnitEventListener(new AdUnitEventListener()
		{
			@Override public void onAdUnitEvent(AD_UNITS adUnit, EVENT_TYPE eventType)
			{
				switch (adUnit) 
				{
					case INTERSTITIAL:
						if (EVENT_TYPE.AD_UNIT_READY == eventType)
						{
							MobileCore.showInterstitial(mActivity, AD_UNIT_SHOW_TRIGGER.APP_START, null);
						}
						break;

					case STICKEEZ:
						// Once Stickeez is ready, calling this method will show the Stickeez on top of the host application. 
						// If the resources are not ready, nothing will happen
						if (EVENT_TYPE.AD_UNIT_READY == eventType) 
						{
							MobileCore.setStickeezPosition(EStickeezPosition.BOTTOM_LEFT);
							MobileCore.showStickee(mActivity);
						}
						break;
				}
			}
		});
	}
  */

//  public void onAccuracyChanged(Sensor sensor, int accuracy)
//  {
 // }

 // public void onSensorChanged(SensorEvent event) 
  //{
      // alpha is calculated as t / (t + dT)
      // with t, the low-pass filter's time-constant
      // and dT, the event delivery rate
/*
      final float alpha = 0.8;

      gravity[0] = alpha * gravity[0] + (1 - alpha) * event.values[0];
      gravity[1] = alpha * gravity[1] + (1 - alpha) * event.values[1];
      gravity[2] = alpha * gravity[2] + (1 - alpha) * event.values[2];

      linear_acceleration[0] = event.values[0] - gravity[0];
      linear_acceleration[1] = event.values[1] - gravity[1];
      linear_acceleration[2] = event.values[2] - gravity[2];*/
    //  Log.e("zzzzzzzzzzz", "a:" + event.values[0]+",b:" + event.values[1]+",c:" + event.values[2]);

  //}

  
}
