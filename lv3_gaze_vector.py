import cv2
import mediapipe as mp
import numpy as np

# -----------------------------
# MediaPipe setup
# -----------------------------
mp_face = mp.solutions.face_mesh
face_mesh = mp_face.FaceMesh(
    static_image_mode=False,
    max_num_faces=1,
    refine_landmarks=True,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)

# -----------------------------
# Camera
# -----------------------------
cap = cv2.VideoCapture(0)

# -----------------------------
# Helpers
# -----------------------------
def normalize(v):
    n = np.linalg.norm(v)
    if n < 1e-6:
        return np.zeros_like(v)
    return v / n

# -----------------------------
# Main loop
# -----------------------------
while True:
    ret, frame = cap.read()
    if not ret:
        break

    h, w, _ = frame.shape
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    result = face_mesh.process(rgb)

    lambda_m = 0.01  # default (no observation)

    if result.multi_face_landmarks:
        face = result.multi_face_landmarks[0]
        lm = face.landmark

        # --- Key landmarks (pixel space) ---
        nose = np.array([lm[1].x * w, lm[1].y * h])
        left_eye_outer  = np.array([lm[33].x * w, lm[33].y * h])
        right_eye_outer = np.array([lm[263].x * w, lm[263].y * h])

        # --- Face-local coordinate frame ---
        face_right = normalize(right_eye_outer - left_eye_outer)
        face_forward = np.array([-face_right[1], face_right[0]])  # 90° rotate

        # --- Eye direction proxy (inside face frame) ---
        eye_center = (left_eye_outer + right_eye_outer) * 0.5
        eye_vec = normalize(eye_center - nose)

        # --- Gaze alignment (distance-invariant) ---
        alignment = np.dot(eye_vec, face_forward)
        lambda_m = np.clip((alignment + 1) * 0.5, 0, 1)

        # --- Draw landmarks (debug) ---
        for p in [nose, left_eye_outer, right_eye_outer]:
            cv2.circle(frame, tuple(p.astype(int)), 4, (0,255,255), -1)

        # --- Draw face forward axis ---
        forward_end = (nose + face_forward * 120).astype(int)
        cv2.arrowedLine(
            frame,
            tuple(nose.astype(int)),
            tuple(forward_end),
            (255, 0, 0),
            3
        )

        # --- Draw eye direction ---
        eye_end = (nose + eye_vec * 120).astype(int)
        cv2.arrowedLine(
            frame,
            tuple(nose.astype(int)),
            tuple(eye_end),
            (0, 0, 255),
            3
        )

    # Overlay λₘ
    cv2.putText(
        frame,
        f"lambda_m: {lambda_m:.3f}",
        (20, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 0, 255),
        2
    )

    cv2.imshow("LV3 Gaze Vector (Face-Local)", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()
