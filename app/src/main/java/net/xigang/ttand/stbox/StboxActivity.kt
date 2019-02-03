package net.xigang.ttand.stbox

import android.app.AlertDialog
import android.app.NativeActivity
import android.content.DialogInterface
import android.content.pm.ApplicationInfo
import android.os.Bundle
import java.util.concurrent.Semaphore

class StboxActivity : NativeActivity() {

    // Use a semaphore to create a modal dialog

    private val semaphore = Semaphore(0, true)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    fun showAlert(message: String) {
        val activity = this

        val applicationInfo = activity.applicationInfo
        val applicationName = applicationInfo.nonLocalizedLabel.toString()

        this.runOnUiThread {
            val builder = AlertDialog.Builder(activity, android.R.style.Theme_Material_Dialog_Alert)
            builder.setTitle(applicationName)
            builder.setMessage(message)
            builder.setPositiveButton("Close") { dialog, id -> semaphore.release() }
            builder.setCancelable(false)
            val dialog = builder.create()
            dialog.show()
        }
        try {
            semaphore.acquire()
        } catch (e: InterruptedException) {
        }

    }

    companion object {

        init {
            // Load native library
            //System.loadLibrary("stbox")
        }
    }
}
