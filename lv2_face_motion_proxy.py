import cv2
import mediapipe as mp
import numpy as np

cap = cv2.VideoCapture(0)

mp_face = mp.solutions.face_detection
face_det = mp_face.FaceDetection(model_selection=0, min_detection_confidence=0.6)

prev_gray = None
lambda_m = 0.0
alpha = 0.15  # smoothing

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    h, w, _ = frame.shape
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    motion_score = 0.0

    if prev_gray is not None:
        diff = cv2.absdiff(gray, prev_gray)
        diff = diff.astype(np.float32) / 255.0

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        res = face_det.process(rgb)

        if res.detections:
            for det in res.detections:
                box = det.location_data.relative_bounding_box
                x1 = int(box.xmin * w)
                y1 = int(box.ymin * h)
                x2 = int((box.xmin + box.width) * w)
                y2 = int((box.ymin + box.height) * h)

                x1, y1 = max(0, x1), max(0, y1)
                x2, y2 = min(w, x2), min(h, y2)

                face_motion = diff[y1:y2, x1:x2].mean()
                motion_score = max(motion_score, face_motion)

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0,255,0), 2)

    lambda_m = (1 - alpha) * lambda_m + alpha * motion_score

    cv2.putText(
        frame,
        f"LV2 λm (face-weighted): {lambda_m:.3f}",
        (20, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 0, 255),
        2
    )

    cv2.imshow("LV2 Face Motion Proxy", frame)
    prev_gray = gray

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()
