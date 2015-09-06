using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using Microsoft.Kinect;

namespace Stuba.BioSandbox
{
    public struct Coordinates2D
    {
        public int X;
        public int Y;
        public int Width;
        public int Height;
        public int TrackingId;
        public int PlayerIndex;

        public int LeftEyeX;
        public int LeftEyeY;

        public int RightEyeX;
        public int RightEyeY;
    }

    struct NativeEngine
    {
        public IntPtr engine;
        public IntPtr inputModule;
        public IntPtr outputModule;
    }

    public class BiometricEngine
    {
        

        // Kinect
        private static readonly Dictionary<int, SkeletonFaceTracker> trackedSkeletons = new Dictionary<int, SkeletonFaceTracker>();
        private const uint MaxMissedFrames = 100;
        private static List<Coordinates2D> coordinates = new List<Coordinates2D>();

        // Native
        private bool _isLoaded = false;
        public bool IsLoaded { get { return _isLoaded; } }

        private NativeEngine _nativeEngine;

        public BiometricEngine() 
        {
            Console.WriteLine("Creating BiometricEngine.");
        }

        ~BiometricEngine()
        {
            Console.WriteLine("Calling destructor.");
        }

        [DllImport("libBioSandbox.dll")]
        private static extern bool ProcessImage(NativeEngine engine, byte[] pixelDataGrayscale, int width, int height, Coordinates2D[] faces, int numFaces, int[] identities);// TODO Here should be a check if there is supported module for inserting images

        [DllImport("libBioSandbox.dll")]
        private static extern bool ProcessImageWithDistances(NativeEngine engine, byte[] pixelData, int width, int height, Coordinates2D[] faces, int numFaces, int[] identities, int numDistances, double[] distances);

        [DllImport("libBioSandbox.dll")]
        private static extern bool ProcessImageWithDepth(NativeEngine engine, byte[] pixelData, int width, int height, short[] depthData, int depthWidth, int depthHeight, Coordinates2D[] faces, int numFaces, int[] identities, int numDistances, double[] distances);

        public void ProcessImage(byte[] pixelDataGrayscale,int width, int height, Coordinates2D [] faces, int [] identities){
            ProcessImage(_nativeEngine, pixelDataGrayscale, width, height, faces, faces.Length, identities );
        }

        public void ProcessImage(byte[] pixelDataGrayscale, int width, int height, Coordinates2D[] faces, int[] identities, double[] distances)
        {
            //if(faces.Length > 0)
            //    Console.WriteLine("Coords: {0},{1},{2},{3}", faces[0].LeftEyeX, faces[0].LeftEyeY, faces[0].RightEyeX, faces[0].RightEyeY);
            ProcessImageWithDistances(
                _nativeEngine, 
                pixelDataGrayscale, 
                width, 
                height, 
                faces, 
                faces.Length, 
                identities, 
                (faces.Length == 0 ? 0 : distances.Length / faces.Length), 
                distances);
        }

        public bool ProcessImage(byte[] pixelDataGrayscale, int width, int height,short [] depthMap, int dWidth,int dHeight, Coordinates2D[] faces, int[] identities, double[] distances)
        {
            //if(faces.Length > 0)
            //    Console.WriteLine("Coords: {0},{1},{2},{3}", faces[0].LeftEyeX, faces[0].LeftEyeY, faces[0].RightEyeX, faces[0].RightEyeY);
            return ProcessImageWithDepth(
                _nativeEngine,
                pixelDataGrayscale,
                width,
                height,
                depthMap,
                dWidth,
                dHeight,
                faces,
                faces.Length,
                identities,
                (faces.Length == 0 ? 0 : distances.Length / faces.Length),
                distances);
        }

        [DllImport("libBioSandbox.dll")]
        private static extern NativeEngine InitializeBiosandbox(string configFile);

        public bool Initialize(string configFile)
        {
            _nativeEngine = InitializeBiosandbox(configFile);
            return _nativeEngine.engine == IntPtr.Zero ? false : true;
        }

        [DllImport("libBioSandbox.dll")]
        private static extern void DeinitializeBiosandbox(NativeEngine engine);

        public void Denitialize()
        {
            DeinitializeBiosandbox(_nativeEngine);
            _nativeEngine.engine = IntPtr.Zero;
        }

        #region Kinect

        private HashSet<int> scannedIdentities = new HashSet<int>();

        public void ProcessFrame(KinectSensor sensor, byte[] colorImage, ColorImageFormat colorImageFormat, DepthImageFrame depthFrame, short[] depthImage, DepthImageFormat depthImageFormat, Skeleton[] skeletonData, SkeletonFrame skeletonFrame)
        {
            //Console.WriteLine("N: ---------");
            coordinates.Clear();
            int detectedFace = 0;
            int trackedSkeletonsCount = 0;
            
            int playerIndex = -1;
            for (int i = 0; i < skeletonData.Length; i++)
            //foreach (Skeleton skeleton in skeletonData)
            {
                Skeleton skeleton = skeletonData[i];
                if (skeleton.TrackingState == SkeletonTrackingState.Tracked
                    || skeleton.TrackingState == SkeletonTrackingState.PositionOnly)
                {
                    // We want keep a record of any skeleton, tracked or untracked.
                    if (!trackedSkeletons.ContainsKey(skeleton.TrackingId))
                    {
                        trackedSkeletons.Add(skeleton.TrackingId, new SkeletonFaceTracker());
                    }

                    DepthImagePoint depthPoint = depthFrame.MapFromSkeletonPoint(skeleton.Joints[JointType.Head].Position);
                    ColorImagePoint colorPoint = depthFrame.MapToColorImagePoint(depthPoint.X, depthPoint.Y, colorImageFormat);
                    
                    Coordinates2D c = new Coordinates2D();

                    playerIndex = i + 1;

                    c.X = colorPoint.X;
                    c.Y = colorPoint.Y;
                    c.Width = 0;
                    c.Height = 0;
                    c.LeftEyeX = 0;
                    c.LeftEyeY = 0;
                    c.RightEyeX = 0;
                    c.RightEyeY = 0;
                    c.PlayerIndex = playerIndex;

                    trackedSkeletonsCount++;

                    // Give each tracker the upated frame.
                    SkeletonFaceTracker skeletonFaceTracker;
                    if (!scannedIdentities.Contains(skeleton.TrackingId) && 
                        detectedFace < 1 &&
                        trackedSkeletons.TryGetValue(skeleton.TrackingId, out skeletonFaceTracker))
                    {
                        detectedFace++;
                        scannedIdentities.Add(skeleton.TrackingId);
                        

                        skeletonFaceTracker.OnFrameReady(sensor, colorImageFormat, colorImage, depthImageFormat, depthImage, skeleton);
                        skeletonFaceTracker.LastTrackedFrame = skeletonFrame.FrameNumber;
                        Coordinates2D? realCoords = skeletonFaceTracker.GetFaceCoordinates();
                        if (realCoords.HasValue)
                        {
                            c = realCoords.Value;
                            c.PlayerIndex = playerIndex;
                        }
                    }

                    c.TrackingId = skeleton.TrackingId;
                    coordinates.Add(c);
                }
            }

            if (scannedIdentities.Count > 0 && scannedIdentities.Count >= trackedSkeletonsCount)
            {
                scannedIdentities.Clear();
                //Console.WriteLine("Clearing");
            }

            RemoveOldTrackers(skeletonFrame.FrameNumber);

            //if (coordinates.Count > 0)
            {
                int[] identities = new int[coordinates.Count];


              //  stopwatch.Reset();
              //  stopwatch.Start();
                double[] distances = new double[coordinates.Count * 8];
                this.
                 ProcessImage(colorImage, GetWidth(colorImageFormat), GetHeight(colorImageFormat), depthImage, 640, 480, coordinates.ToArray(), identities, distances);
              //  stopwatch.Stop();
         //       foreach (int i in identities)
         //       {
         //           Console.WriteLine("Recognized: {0} (in {1} millis - {2} ticks)", i, stopwatch.ElapsedMilliseconds, stopwatch.ElapsedTicks);
         //       }
            }

        }

        private static Stopwatch stopwatch = new Stopwatch();

        private Skeleton GetTrackedSkeleton(Skeleton[] skeletons)
        {
            foreach (Skeleton skeleton in skeletons)
            {
                if (SkeletonTrackingState.Tracked == skeleton.TrackingState)
                {
                    return skeleton;
                }
            }
            return null;
        }

        /// <summary>
        /// Clear out any trackers for skeletons we haven't heard from for a while
        /// </summary>
        private void RemoveOldTrackers(int currentFrameNumber)
        {
            var trackersToRemove = new List<int>();

            foreach (var tracker in trackedSkeletons)
            {
                uint missedFrames = (uint)currentFrameNumber - (uint)tracker.Value.LastTrackedFrame;
                if (missedFrames > MaxMissedFrames)
                {
                    // There have been too many frames since we last saw this skeleton
                    trackersToRemove.Add(tracker.Key);
                }
            }

            foreach (int trackingId in trackersToRemove)
            {
                RemoveTracker(trackingId);
            }
        }

        private void RemoveTracker(int trackingId)
        {
            trackedSkeletons[trackingId].Dispose();
            trackedSkeletons.Remove(trackingId);
        }

        public void ResetFaceTracking()
        {
            foreach (int trackingId in new List<int>(trackedSkeletons.Keys))
            {
                RemoveTracker(trackingId);
            }
        }
        #endregion

        public static int GetWidth(ColorImageFormat format)
        {
            switch (format) {
                case ColorImageFormat.RgbResolution1280x960Fps12:
                    return 1280;
                case ColorImageFormat.RawYuvResolution640x480Fps15:
                case ColorImageFormat.RgbResolution640x480Fps30:
                case ColorImageFormat.YuvResolution640x480Fps15:
                    return 640;
                default:
                    return 640;
            }
        }

        public static int GetHeight(ColorImageFormat format)
        {
            switch (format)
            {
                case ColorImageFormat.RgbResolution1280x960Fps12:
                    return 960;
                case ColorImageFormat.RawYuvResolution640x480Fps15:
                case ColorImageFormat.RgbResolution640x480Fps30:
                case ColorImageFormat.YuvResolution640x480Fps15:
                    return 480;
                default:
                    return 480;
            }
        }
    }



    public class SkeletonFaceTracker : IDisposable
    {
        private static FaceTriangle[] faceTriangles;

        private EnumIndexableCollection<FeaturePoint, PointF> facePoints;

        private FaceTracker faceTracker;

        private bool lastFaceTrackSucceeded;

        private SkeletonTrackingState skeletonTrackingState;

        public int LastTrackedFrame { get; set; }

        public void Dispose()
        {
            if (this.faceTracker != null)
            {
                this.faceTracker.Dispose();
                this.faceTracker = null;
            }
        }

        /// <summary>
        /// Updates the face tracking information for this skeleton
        /// </summary>
        public void OnFrameReady(KinectSensor kinectSensor, ColorImageFormat colorImageFormat, byte[] colorImage, DepthImageFormat depthImageFormat, short[] depthImage, Skeleton skeletonOfInterest)
        {
            this.skeletonTrackingState = skeletonOfInterest.TrackingState;

            if (this.skeletonTrackingState != SkeletonTrackingState.Tracked)
            {
                // nothing to do with an untracked skeleton.
                return;
            }

            if (this.faceTracker == null)
            {
                try
                {
                    this.faceTracker = new FaceTracker(kinectSensor);
                }
                catch (InvalidOperationException)
                {
                    // During some shutdown scenarios the FaceTracker
                    // is unable to be instantiated.  Catch that exception
                    // and don't track a face.
                    Debug.WriteLine("AllFramesReady - creating a new FaceTracker threw an InvalidOperationException");
                    this.faceTracker = null;
                }
            }

            if (this.faceTracker != null)
            {
                FaceTrackFrame frame = this.faceTracker.Track(
                    colorImageFormat, colorImage, depthImageFormat, depthImage, skeletonOfInterest);

                this.lastFaceTrackSucceeded = frame.TrackSuccessful;
                if (this.lastFaceTrackSucceeded)
                {
                    if (faceTriangles == null)
                    {
                        // only need to get this once.  It doesn't change.
                        faceTriangles = frame.GetTriangles();
                    }

                    this.facePoints = frame.GetProjected3DShape();
                }
            }
        }
        public Coordinates2D? GetFaceCoordinates()
        {
            return faceTracker == null ? null : faceTracker.GetFaceCoordinates();
        }
    }

}
