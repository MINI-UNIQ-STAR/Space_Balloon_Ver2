// GPS XA1110 UART Sensor for Renode
// Sierra Wireless XA1110 - GNSS Module with NMEA Output
// Renode-compatible UART implementation

using System;
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.UART;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class XA1110_GPS : IUART
    {
        public XA1110_GPS()
        {
            // Initial position: South Korea
            latitude = 35.0929800;
            longitude = 126.9988700;
            altitude = 0.0;
            
            // Satellite counts
            gpsSatellites = 8;
            glonassSatellites = 6;
            galileoSatellites = 4;
            beidouSatellites = 3;
            
            // Speed and heading
            speedKnots = 0.0;
            heading = 0.0;
            
            outputBuffer = new Queue<byte>();
            
            this.Log(LogLevel.Info, "XA1110 GPS Reset - Position: {0:F6}N, {1:F6}E, {2:F1}m", 
                latitude, longitude, altitude);
        }

        public void WriteChar(byte value)
        {
            // GPS receives commands (PMTK messages)
            // For simulation, we ignore incoming data
        }

        public void Reset()
        {
            time = 0;
            altitude = 0.0;
            outputBuffer.Clear();
            this.Log(LogLevel.Info, "XA1110 GPS Reset");
        }

        public event Action<byte> CharReceived;

        // Generate and send NMEA sentences (call this periodically)
        public void GenerateNMEA()
        {
            UpdateSimulation();
            
            // Generate all NMEA sentences
            string gga = GenerateGGA();
            string rmc = GenerateRMC();
            string gsa = GenerateGSA();
            string gsv_gps = GenerateGSV("GP", gpsSatellites);
            string gsv_glonass = GenerateGSV("GL", glonassSatellites);
            
            // Queue all sentences
            QueueString(gga);
            QueueString(rmc);
            QueueString(gsa);
            QueueString(gsv_gps);
            QueueString(gsv_glonass);
            
            // Send queued data
            SendQueuedData();
        }

        private void UpdateSimulation()
        {
            time += 1.0;
            
            // Simulate balloon ascent: 5 m/s
            altitude += 5.0;
            if (altitude > 40000.0) altitude = 40000.0;
            
            // Slight drift in position (wind effect)
            double windAngle = time * 0.01;
            latitude += 0.00001 * Math.Cos(windAngle);
            longitude += 0.00002 * Math.Sin(windAngle);
            
            // Slight rotation (balloon spin)
            heading += 0.5;
            if (heading >= 360.0) heading -= 360.0;
            
            // Add some noise to satellite count at high altitude
            if (altitude > 30000)
            {
                // Ionospheric effects at high altitude
                gpsSatellites = 6 + random.Next(3);
                glonassSatellites = 4 + random.Next(3);
            }
        }

        private string GenerateGGA()
        {
            // $GPGGA - Global Positioning System Fix Data
            var utc = DateTime.UtcNow;
            string timeStr = utc.ToString("HHmmss.00");
            
            string latStr = FormatLatitude(latitude);
            string latDir = latitude >= 0 ? "N" : "S";
            string lonStr = FormatLongitude(longitude);
            string lonDir = longitude >= 0 ? "E" : "W";
            
            int totalSats = gpsSatellites + glonassSatellites + galileoSatellites + beidouSatellites;
            
            string sentence = string.Format(
                "$GPGGA,{0},{1},{2},{3},{4},1,{5:00},1.0,{6:F1},M,0.0,M,,",
                timeStr, latStr, latDir, lonStr, lonDir, totalSats, altitude);
            
            return AddChecksum(sentence);
        }

        private string GenerateRMC()
        {
            // $GPRMC - Recommended Minimum Navigation Information
            var utc = DateTime.UtcNow;
            string timeStr = utc.ToString("HHmmss.00");
            string dateStr = utc.ToString("ddMMyy");
            
            string latStr = FormatLatitude(latitude);
            string latDir = latitude >= 0 ? "N" : "S";
            string lonStr = FormatLongitude(longitude);
            string lonDir = longitude >= 0 ? "E" : "W";
            
            string sentence = string.Format(
                "$GPRMC,{0},A,{1},{2},{3},{4},{5:F1},{6:F1},{7},,",
                timeStr, latStr, latDir, lonStr, lonDir, speedKnots, heading, dateStr);
            
            return AddChecksum(sentence);
        }

        private string GenerateGSA()
        {
            // $GPGSA - GPS DOP and active satellites
            // Mode: A=Auto, Fix: 3=3D fix
            string sentence = "$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,2.0,1.0,1.7";
            return AddChecksum(sentence);
        }

        private string GenerateGSV(string prefix, int satCount)
        {
            // $GPGSV or $GLGSV - Satellites in view
            // Simplified: just report count
            string sentence = string.Format(
                "${0}GSV,1,1,{1:00},01,45,090,45,02,30,180,40,03,60,270,50,04,15,045,35",
                prefix, satCount);
            return AddChecksum(sentence);
        }

        private string FormatLatitude(double lat)
        {
            double absLat = Math.Abs(lat);
            int degrees = (int)absLat;
            double minutes = (absLat - degrees) * 60.0;
            return string.Format("{0:00}{1:07.4f}", degrees, minutes);
        }

        private string FormatLongitude(double lon)
        {
            double absLon = Math.Abs(lon);
            int degrees = (int)absLon;
            double minutes = (absLon - degrees) * 60.0;
            return string.Format("{0:000}{1:07.4f}", degrees, minutes);
        }

        private string AddChecksum(string sentence)
        {
            // Calculate XOR checksum of everything between $ and *
            int checksum = 0;
            for (int i = 1; i < sentence.Length; i++)
            {
                checksum ^= sentence[i];
            }
            return sentence + "*" + checksum.ToString("X2") + "\r\n";
        }

        private void QueueString(string s)
        {
            foreach (char c in s)
            {
                outputBuffer.Enqueue((byte)c);
            }
        }

        private void SendQueuedData()
        {
            while (outputBuffer.Count > 0)
            {
                byte b = outputBuffer.Dequeue();
                CharReceived?.Invoke(b);
            }
        }

        // IUART interface properties
        public uint BaudRate => 9600;
        public Parity ParityBit => Parity.None;
        public Bits StopBits => Bits.One;

        // Position data
        private double latitude;
        private double longitude;
        private double altitude;
        private double speedKnots;
        private double heading;
        private double time;
        
        // Satellite data
        private int gpsSatellites;
        private int glonassSatellites;
        private int galileoSatellites;
        private int beidouSatellites;
        
        private Queue<byte> outputBuffer;
        private Random random = new Random();
    }
}
