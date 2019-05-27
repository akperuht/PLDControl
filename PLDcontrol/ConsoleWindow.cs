using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PLDcontrol
{
    /// <summary>
    /// Form to show logfile content
    /// </summary>
    public partial class ConsoleWindow : Form
    {
        // Threading for setting text to richtextbox
        delegate void SetTextCallback(string text);
        private Thread logThread = null;
        private string filename = "pldcontrol_log_file.aki";
        private string path;
        // Waiting time for logfile lock
        private const int ASYNC_DELAY = 5;

        // Lock object to tell wether file is written in main form or not
        private dataTransfer fileLock;

        // Watcher for logfile changes
        private FileSystemWatcher watcher;


        /// <summary>
        /// Constructor for console window class
        /// </summary>
        /// <param name="mainlock">object to pass boolean value for file locking</param>
        public ConsoleWindow(dataTransfer mainlock)
        {
            fileLock = mainlock; // locking object for logfile
            System.Console.WriteLine(path + filename);
            path = Path.Combine(fileLock.filepath, filename);
            InitializeComponent();
            InitializeLogging();
            // Closing created filesystem watcher when form closes. Not necessary but to be sure
            this.FormClosed += delegate { watcher.EnableRaisingEvents = false; watcher.Dispose(); };
        }

        /// <summary>
        /// Initializes reading from logfile
        /// </summary>
        private void InitializeLogging()
        {
            // Creating watcher to detect changes in logfile
            watcher = new FileSystemWatcher();
            watcher.Path = fileLock.filepath;
            watcher.NotifyFilter = NotifyFilters.LastWrite;
            watcher.Filter = "*.aki"; // logfile extension is .aki, the best extension ever
            watcher.Changed += delegate {ReadLogfile(); }; // Raises event when logfile is changed
            watcher.EnableRaisingEvents = true;
            ReadLogfile();
        }

        /// <summary>
        /// Reads logfile asyncrhonously
        /// </summary>
        private async void ReadLogfile()
        {
            if (fileLock != null)// Check if filelock object is created, kinda useless check
            {
            while (fileLock.locking) { await Task.Delay(ASYNC_DELAY); }// waiting logfile to unlock
                if (!fileLock.locking)
                {
                    fileLock.locking = true; // locking the logfile during reading
                    using (System.IO.StreamReader file = new System.IO.StreamReader(path + filename))
                    {
                        string text = file.ReadToEnd();
                        SetText(text); // setting text to richtextbox in another thread
                    }
                    fileLock.locking = false; // unlocking the logfile
                }
            }
        }

        ///
        /// Threading for updating textbox
        /// 
        private void updateLogText()
        {
            logThread = new Thread(new ThreadStart(this.ReadLogfile)); // Making new thread
            logThread.IsBackground = true; // Making thread to run background
            logThread.Start();
        }

        // If the calling thread is different from the thread that
        // created the TextBox control, this method creates a
        // SetTextCallback and calls itself asynchronously using the
        // Invoke method.
        //
        // If the calling thread is the same as the thread that created
        // the TextBox control, the Text property is set directly. 
        //
        // If text is updated very frequently, this may cause flickering of richTextBox
        private void SetText(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.richTextBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                try { this.Invoke(d, new object[] { text}); }
                catch (ObjectDisposedException){ /**/ }
            }
            else
            {
                this.richTextBox1.Clear(); // clears previous text
                
                // Extracting rows from text
                string[] rows = text.Split(new string[] { "\n\r\n" }, StringSplitOptions.None);

                // Processing each row
                foreach (string row in rows)
                {

                    // Extracting words from row
                    string[] words = row.Split();
                    bool start = true;

                    // Appending each word separately to richtextbox
                    foreach (string word in words)
                    {
                        try
                        {
                            // Setting text to richtextbox
                            richTextBox1.SelectionStart = richTextBox1.TextLength;
                            richTextBox1.SelectionLength = 0;

                            // Selecting word colours
                            if (word.Trim().StartsWith("PLD")) { richTextBox1.SelectionColor = Color.Aqua; }
                            else if (word.Trim() == "Device:") { richTextBox1.SelectionColor = Color.Lime; }
                            else if (word.Trim() == "PC->NL:") { richTextBox1.SelectionColor = Color.Crimson; }
                            else if (word.Trim() == "NL->PC:") { richTextBox1.SelectionColor = Color.Silver; }
                            else if (word.Trim() == "Logfile" || word.Trim() == "viewing" || word.Trim() == "started") { richTextBox1.SelectionColor = Color.Orange; }
                            else if (start) { richTextBox1.SelectionColor = Color.YellowGreen; start = false; }
                            else { richTextBox1.SelectionColor = Color.White; }

                            // Appending word to richtextbox
                            richTextBox1.AppendText(word + " ");
                            richTextBox1.SelectionColor = richTextBox1.ForeColor;
                        }
                        catch(Exception ex)
                        {
                            System.Console.WriteLine(ex);
                        }
                    }
                    // Ending the line outside loop, and catching exception if richtextbox does not exist anymore
                    try { richTextBox1.AppendText("\n"); } catch (ObjectDisposedException ex) { watcher.EnableRaisingEvents = false;/*Disabling event raising capability of file watcher*/ }
                }
            }
        }

        /* 
         =============================================================
         Useless event handlers
         =============================================================
         */

            // Handler for textbox
        private void richTextBox1_TextChanged(object sender, EventArgs e)
        {
            //
        }
    }
}
