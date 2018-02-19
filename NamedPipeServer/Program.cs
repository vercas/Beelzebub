using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.IO.Pipes;
using System.Threading;

namespace NamedPiper {
	class Program {
		static void Main(string[] args) {
			while (true) {
				try {
					Do();
				} catch (Exception E) {
					Console.WriteLine("Exception: {0}", E.Message);
				}
			}

			Console.WriteLine("Done!");
			Console.ReadLine();
		}

		static void Do() {
			Console.WriteLine("Starting pipe");
			Random Rnd = new Random();

			NamedPipeClientStream PipeClient = new NamedPipeClientStream("Aladin");
			PipeClient.Connect();
			Console.WriteLine("Connected! Sending random garbage ...");

			BinaryWriter Writer = new BinaryWriter(PipeClient);

			while (true) {
				Thread.Sleep(Rnd.Next(50, 1100));

				Writer.Write((byte)0x10);

				string Str = "Hello World #" + Rnd.Next(1000000, 9999999);
				byte[] Data = Encoding.UTF8.GetBytes(Str);
				Writer.Write((int)Data.Length);
				Writer.Write(Data);

				PipeClient.WaitForPipeDrain();
			}
		}
	}
}
