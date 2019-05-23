using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PLDcontrol
{
    public partial class Info : Form
    {
        public Info()
        {
            InitializeComponent();
            progressBar1.PerformStep();
            progressBar1.Show();
            progressBar1.Step = 100;
            progressBar1.PerformStep();
            System.Windows.Forms.Timer timer2 = new System.Windows.Forms.Timer();
            timer2.Interval = 1;
            timer2.Start();
            timer2.Tick += delegate {
                progressBar1.PerformStep();
                progressBar1.Update();
            };
            System.Windows.Forms.Timer timer = new System.Windows.Forms.Timer();
            timer.Interval = 2000;
            timer.Start();
            timer.Tick += delegate { Close(); };
        }

        private void Info_Load(object sender, EventArgs e)
        {
            // Loading the info form
        }

        private void progressBar1_Click(object sender, EventArgs e)
        {
            // Click of the progress bar
        }
    }
}
