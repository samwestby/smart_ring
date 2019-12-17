package com.example.bluetooth_alarm;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
//import android.support.v7.app.ActionBarActivity;
import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.media.AudioManager;
import android.provider.MediaStore;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.widget.EditText;
import android.widget.TextView;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.util.UUID;


public class ledControl extends AppCompatActivity {

   // Button btnOn, btnOff, btnDis;
    Button On, Off, Discnt, Abt;
    TextView myTextbox;
    String address = null;
    private ProgressDialog progress;
    BluetoothAdapter myBluetooth = null;
    BluetoothSocket btSocket = null;
//    BluetoothServerSocket mmServerSocket = null;
    private boolean isBtConnected = false;
//    private String lastGesture = null;
    Thread workerThread;
//    OutputStream mmOutputStream;
    InputStream mmInputStream;
    byte[] readBuffer;
    int readBufferPosition;
//    int counter;
    volatile boolean stopWorker;
    //SPP UUID. Look for it
    static final UUID myUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");


    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Intent newint = getIntent();
        address = newint.getStringExtra(DeviceList.EXTRA_ADDRESS);

        setContentView(R.layout.activity_led_control);

        Discnt = (Button)findViewById(R.id.dis_btn);
        Abt = (Button)findViewById(R.id.abt_btn);
        myTextbox = (TextView) findViewById(R.id.entry);


        new ConnectBT().execute();

        //commands to be sent to bluetooth
//        On.setOnClickListener(new View.OnClickListener()
//        {
//            @Override
//            public void onClick(View v)
//            {
//                turnOnLed();      //method to turn on
////                g0.start();
//            }
//        });
//
//        Off.setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View v)
//            {
//                turnOffLed();   //method to turn off
//
//            }
//        });

        Discnt.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                Disconnect(); //close connection


            }
        });

        Abt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Abt.setText("Started");
                beginListenForData();
            }
        });


    }

    private void Disconnect()
    {
        if (btSocket!=null) //If the btSocket is busy
        {
            try
            {
                btSocket.close(); //close connection
            }
            catch (IOException e)
            { msg("Error");}
        }
        finish(); //return to the first layout

    }

    private void turnOffLed()
    {
        if (btSocket!=null)
        {
            try
            {
                btSocket.getOutputStream().write("0".getBytes());
            }
            catch (IOException e)
            {
                msg("Error");
            }
        }
    }

    private void turnOnLed()
    {
//        Log.d("fda", "Start");
        if (btSocket!=null)
        {
//            Log.d("fda", "btSocket");
            try
            {
//                Log.d("try", "try");
                btSocket.getOutputStream().write("1".getBytes());
            }
            catch (IOException e)
            {
                msg("Error");
            }
        }

    }

    private MediaPlayer gestureControls(MediaPlayer mps, String key, AudioManager audioManager){
        key = key.replaceAll("\\s+", "");
        switch (key){
            case "0":
                mps.pause();
                mps.reset();
//                mps = MediaPlayer.create(this, R.raw.siren);
                break;
            case "1":
//                mps.reset();
                mps = MediaPlayer.create(this, R.raw.siren);
                mps.start();
                break;
            case "2":
//                mps[2].start();
                audioManager.adjustVolume(AudioManager.ADJUST_RAISE, AudioManager.FLAG_PLAY_SOUND);
                break;
            case "3":
//                mps[3].start();
                audioManager.adjustVolume(AudioManager.ADJUST_LOWER, AudioManager.FLAG_PLAY_SOUND);
                break;
        }
        return mps;
    }

    private void beginListenForData()
    {
        if (null != workerThread && workerThread.isAlive()){
            return;
        }
        final Handler handler = new Handler();
        final MediaPlayer g2 = MediaPlayer.create(this, R.raw.siren);
        final AudioManager audioManager = (AudioManager) getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
        stopWorker = false;
        readBufferPosition = 0;
        readBuffer = new byte[4];

        workerThread = new Thread(new Runnable()
        {
            public void run()
            {
                MediaPlayer g1 = g2;
                while(!Thread.currentThread().isInterrupted() && !stopWorker)
                {

//                    Log.d("a", "Thread BABYYY");
                    try{
                        mmInputStream = btSocket.getInputStream();
                    }catch(IOException e){
                        Log.d("a", "No input stream");
                        continue;
                    }
                    try
                    {
                        int bytesAvailable = mmInputStream.available();
                        if(bytesAvailable > 0)
                        {
                            DataInputStream mmInStream = new DataInputStream(mmInputStream);
                            byte[] buffer  = new byte[bytesAvailable];
                            int bytes;
                            bytes = mmInStream.read(buffer);
                            String message = new String(buffer, 0, bytes);
                            message = message.replaceAll("\\s+", "");
                            Log.d("a", message);
                            g1 = gestureControls(g1, message, audioManager);
                            switch (message) {
                                case "0":
                                    message = "Pause";
                                    break;
                                case "1":
                                    message = "Play";
                                    break;
                                case "2":
                                    message = "Volume Up";
                                    break;
                                case "3":
                                    message = "Volume Down";
                                    break;
                            }
                            final String MESSAGE = message;
                            handler.post(new Runnable()
                            {
                                public void run()
                                {

                                    myTextbox.setText(MESSAGE);
                                }
                            });
                        }
                    }
                    catch (IOException ex)
                    {
                        stopWorker = true;
                    }

                }
            }
        });

        workerThread.start();


    }

    /*
    void sendData() throws IOException
    {
        String msg = myTextbox.getText().toString();
        msg += "\n";
        mmOutputStream.write(msg.getBytes());
        myTextbox.setText("Data Sent");
    }

    void closeBT() throws IOException
    {
        stopWorker = true;
        mmOutputStream.close();
        mmInputStream.close();
        btSocket.close();
        myTextbox.setText("Bluetooth Closed");
    }

     */

    private String getRingInput(BluetoothSocket btSocket){
        InputStream inputString = null;
        String message = "";
        try {
            inputString = btSocket.getInputStream();
//            Log.d("hi", Integer.toString(inputString.available()));
            byte[] buffer = new byte[100];
            inputString.read(buffer);
            inputString.mark(1);
            message = new String(buffer);
            Log.d("hi", message);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return message;
    }

    // fast way to call Toast
    private void msg(String s)
    {
        Toast.makeText(getApplicationContext(),s, Toast.LENGTH_LONG).show();
    }

//    public void about(View v)
//    {
//        if(v.getId() == R.id.abt)
//        {
//            Intent i = new Intent(this, AboutActivity.class);
//            startActivity(i);
//        }
//    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_led_control, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }



    private class ConnectBT extends AsyncTask<Void, Void, Void>  // UI thread
    {
        private boolean ConnectSuccess = true; //if it's here, it's almost connected

        @Override
        protected void onPreExecute()
        {
            progress = ProgressDialog.show(ledControl.this, "Connecting...", "Please wait!!!");  //show a progress dialog
        }

        @Override
        protected Void doInBackground(Void... devices) //while the progress dialog is shown, the connection is done in background
        {
            try
            {
                if (btSocket == null || !isBtConnected)
                {
                    myBluetooth = BluetoothAdapter.getDefaultAdapter();//get the mobile bluetooth device
                    BluetoothDevice dispositivo = myBluetooth.getRemoteDevice(address);//connects to the device's address and checks if it's available
                     btSocket = dispositivo.createInsecureRfcommSocketToServiceRecord(myUUID);//create a RFCOMM (SPP) connection
                     BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
                     btSocket.connect();//start connection
////                 mmServerSocket = myBluetooth.listenUsingRfcommWithServiceRecord("MyService", myUUID);
//                    mmServerSocket =  myBluetooth.listenUsingInsecureRfcommWithServiceRecord("MyService", myUUID); // you can also try  using In Secure connection...
//                    // This is a blocking call and will only return on a
//                // successful connection or an exceptio
//                    btSocket = mmServerSocket.accept();
                }
            }
            catch (IOException e)
            {
                ConnectSuccess = false;//if the try failed, you can check the exception here
            }
            return null;
        }
        @Override
        protected void onPostExecute(Void result) //after the doInBackground, it checks if everything went fine
        {
            super.onPostExecute(result);

            if (!ConnectSuccess)
            {
                msg("Connection Failed. Is it a SPP Bluetooth? Try again.");
                finish();
            }
            else
            {
                msg("Connected.");
                isBtConnected = true;
            }
            progress.dismiss();

//            beginListenForData();
        }
    }
}
