#pragma once
#include "CPPUtil.h"

class GameWorld {
public:
	float WorldWidth;
	float WorldHeight;

	GameWorld(float width, float height) : WorldWidth(width), WorldHeight(height) {}

};


class PhysicsBox {
public:
	PhysicsBox(Util::RECTF rect, Util::Vector2 startingVelocity, float startingOrientation, float startingRotationalVelocity)
	{
		currentPosition = rect.GetCenter();
		boundingRect = Util::RECTF(rect.Left - currentPosition.X, rect.Top - currentPosition.Y, rect.Right - currentPosition.X, rect.Bottom - currentPosition.Y);
		currentVelocity = startingVelocity;
		currentOrientation = startingOrientation;
		currentRotationalVelocity = startingRotationalVelocity;
	}

	float GetWidth() const {
		return boundingRect.Width;
	}

	float GetHeight() const {
		return boundingRect.Height;
	}

	void RunPhysicsStep(const GameWorld& world) {
		WorldBoundaryHit(world);
		ApplyRotation();
		ApplyVelocity();
	}

	void WorldBoundaryHit(const GameWorld& world) {
		float aspectRatioX = world.WorldWidth;
		float aspectRatioY = world.WorldHeight;

		Util::Vector2 scaledTopLeft = (currentPosition + boundingRect.GetTopLeft()) * Util::Vector2(aspectRatioX, aspectRatioY);
		Util::Vector2 scaledTopRight = (currentPosition + boundingRect.GetTopRight()) * Util::Vector2(aspectRatioX, aspectRatioY);
		Util::Vector2 scaledBottomLeft = (currentPosition + boundingRect.GetBottomLeft()) * Util::Vector2(aspectRatioX, aspectRatioY);
		Util::Vector2 scaledBottomRight = (currentPosition + boundingRect.GetBottomRight()) * Util::Vector2(aspectRatioX, aspectRatioY);

		Util::Vector2 rotatedTopLeft = Util::Vector2::RotatePoint(scaledTopLeft, currentPosition * Util::Vector2(aspectRatioX, aspectRatioY), currentOrientation);
		Util::Vector2 rotatedTopRight = Util::Vector2::RotatePoint(scaledTopRight, currentPosition * Util::Vector2(aspectRatioX, aspectRatioY), currentOrientation);
		Util::Vector2 rotatedBottomLeft = Util::Vector2::RotatePoint(scaledBottomLeft, currentPosition * Util::Vector2(aspectRatioX, aspectRatioY), currentOrientation);
		Util::Vector2 rotatedBottomRight = Util::Vector2::RotatePoint(scaledBottomRight, currentPosition * Util::Vector2(aspectRatioX, aspectRatioY), currentOrientation);

		rotatedTopLeft /= Util::Vector2(aspectRatioX, aspectRatioY);
		rotatedTopRight /= Util::Vector2(aspectRatioX, aspectRatioY);
		rotatedBottomLeft /= Util::Vector2(aspectRatioX, aspectRatioY);
		rotatedBottomRight /= Util::Vector2(aspectRatioX, aspectRatioY);

		Util::Vector2 topLeft = rotatedTopLeft;
		Util::Vector2 topRight = rotatedTopRight;
		Util::Vector2 bottomLeft = rotatedBottomLeft;
		Util::Vector2 bottomRight = rotatedBottomRight;

		float rotationalImpact = 1;

		// Collision checks using rotated rectangle's corners
		for (int i = 0; i < 4; ++i)
		{
			Util::Vector2 Corner;
			if (i == 0) { Corner = topLeft; }
			else if (i == 1) { Corner = topRight; }
			else if (i == 2) { Corner = bottomRight; }
			else { Corner = bottomLeft; }
			if (Util::RECTF(0, 0, 1, 1).Contains(Corner)) { continue; } //no collision

			float adjustedOrientation = currentOrientation + i * 90;
			if (adjustedOrientation >= 360) { adjustedOrientation -= 360; }

			if (Corner.X <= 0 || Corner.X >= 1) //horizontal wall hit
			{
				if (Corner.X <= 0) { currentVelocity.X = abs(currentVelocity.X); }
				else { currentVelocity.X = -abs(currentVelocity.X); }
				float distance_from_threshold = min(abs(adjustedOrientation - 315), abs(adjustedOrientation - 135));
				float scaling_factor = distance_from_threshold / 45;
				if (adjustedOrientation > 315 || adjustedOrientation < 135) //Top Left corner is on the upper part of the rectangle
				{
					if (Corner.X >= 1) { currentRotationalVelocity -= (1 * scaling_factor); } //counterclockwise rotational force applied
					else if (Corner.X <= 0) { currentRotationalVelocity += (1 * scaling_factor); } //clockwise force applied
				}
				else //Top Left corner is on the lower part of the rectangle
				{
					if (Corner.X >= 1) { currentRotationalVelocity += (1 * scaling_factor); } //clockwise force applied
					if (Corner.X <= 0) { currentRotationalVelocity -= (1 * scaling_factor); } //counterclockwise force applied
				}
			}
			if (Corner.Y <= 0 || Corner.Y >= 1) //vertical wall hit
			{
				if (Corner.Y <= 0) { currentVelocity.Y = abs(currentVelocity.Y); }
				else { currentVelocity.Y = -abs(currentVelocity.Y); }
				float distance_from_threshold = min(abs(adjustedOrientation - 225), abs(adjustedOrientation - 45));
				float scaling_factor = distance_from_threshold / 45;
				if (adjustedOrientation < 45 || adjustedOrientation > 225) //Top Left corner is on the left side of the rectangle
				{
					if (Corner.Y <= 0) { currentRotationalVelocity -= (1 * scaling_factor); } //counterclockwise force applied
					else if (Corner.Y >= 1) { currentRotationalVelocity += (1 * scaling_factor); } //clockwise force applied
				}
				else //top left corner is on the right side of the rectangle
				{
					if (Corner.Y <= 0) { currentRotationalVelocity += (1 * scaling_factor); } //clockwise force applied
					else if (Corner.Y >= 1) { currentRotationalVelocity -= (1 * scaling_factor); } //counterclockwise force applied
				}
			}
		}
	}

	void ApplyRotation() {
		currentOrientation += currentRotationalVelocity;
		if (currentOrientation > 360.0f) {
			currentOrientation -= 360.0f;
		}
		else if (currentOrientation < 0.0f) {
			currentOrientation += 360.0f;
		}
	}

	void ApplyVelocity() {
		currentPosition += currentVelocity;
	}

	Util::RECTF GetWorldRect() const {
		return Util::RECTF(
			boundingRect.Left + currentPosition.X, boundingRect.Top + currentPosition.Y,
			boundingRect.Right + currentPosition.X, boundingRect.Bottom + currentPosition.Y
		);
	}

	Util::Vector2 GetPosition() const {
		return currentPosition;
	}

	float GetOrientation() const {
		return currentOrientation;
	}

private:
	Util::RECTF boundingRect;
	Util::Vector2 currentPosition;
	float currentOrientation;
	Util::Vector2 currentVelocity;
	float currentRotationalVelocity;
};
