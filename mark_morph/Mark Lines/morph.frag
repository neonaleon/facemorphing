#extension GL_ARB_texture_rectangle : require

uniform sampler2DRect TexA;		// Input texture A
uniform sampler2DRect TexB;		// Input texture B

uniform sampler2DRect ALines;	// Input texture A
uniform sampler2DRect BLines;	// Input texture B

uniform float Step;
uniform float LineCount;
uniform float TexWidth;
uniform float TexHeight;
uniform float BlendType;

const float a = 0.5;			// smoothness of warping
const float b = 3.25;			// relative line strength
const float p = 0.25;

const float Epsilon = 0.0000001;

//------------------------------------------------------------------------------
// Function name: calcXPrime
// Parameters:
//		-Pprime: the source line start point
//		-Qprime: the source line end point
//		-uv: coordinates in feature line space
// Return:
//		The distance along the feature line
// Description:
//		Uses the Beier-Neely equation to calculate the new pixel value due 
//		to warping using various vector operations.	If the new value is 
//		out of the range of the image dimensions, the current value is used
//------------------------------------------------------------------------------
vec2 calcXPrime(vec2 Pprime, vec2 Qprime, vec2 uv)
{
	vec2 QprimePprime = Qprime - Pprime;
	vec2 QprimePprimePerp = QprimePprime.yx;
	QprimePprimePerp.x = -QprimePprimePerp.x;
	return Pprime + QprimePprime * uv.x + QprimePprimePerp * uv.y / length(QprimePprime);
}

//------------------------------------------------------------------------------
// Function name: calcU
// Parameters:
//		-X: the current destination image pixel
//		-P: the destination line start point
//		-Q: the destination line end point
// Return:
//		The distance along the feature line
// Description:
//		Uses the Beier-Neely equation to calculate the distance along the 
//		feature line using various vector operations.	The value is between
//		0 and 1 if it lies along the line, otherwise u is outside the range
//------------------------------------------------------------------------------
float calcU(vec2 X, vec2 P, vec2 Q)
{
	vec2 PX = X - P;
	vec2 PQ = Q - P;
	return dot(PX, PQ) / dot(PQ, PQ);
}

//------------------------------------------------------------------------------
// Function name: calcV
// Parameters:
//		-X: the current destination image pixel
//		-P: the destination line start point
//		-Q: the destination line end point
// Return:
//		The perpendicular distance from the current pixel to the line
// Description:
//		Uses the Beier-Neely equation to calculate the perpendicular distance
//		of the current destination pixel to the current feature line using 
//		various vector operations
//------------------------------------------------------------------------------
float calcV(vec2 X, vec2 P, vec2 Q)
{
	vec2 PX = X - P;
	vec2 PQ = Q - P;
	vec2 PQperp = PQ.yx;
	PQperp.x = -PQperp.x;
	return dot(PX, PQperp) / length(PQ);
}

void main()
{
	float weightsum = 0.0;
	vec2 dsumA, dsumB;
	dsumA = dsumB = vec2(0);
	vec2 X = gl_FragCoord.xy; 

	for(float i=0.5; i<LineCount; i++)
	{
		vec4 interpLine = mix(texture2DRect(ALines, vec2(i, 0.5)), texture2DRect(BLines, vec2(i, 0.5)), Step);
		vec2 P = interpLine.xy;
		vec2 Q = interpLine.zw;
		
		vec2 uv;
		uv.x = calcU(X, P, Q);
		uv.y = calcV(X, P, Q);
		vec2 PprimeA = texture2DRect(ALines, vec2(i, 0.5)).xy;
		vec2 QprimeA = texture2DRect(ALines, vec2(i, 0.5)).zw;
		vec2 XprimeA = calcXPrime(PprimeA, QprimeA, uv);
		vec2 PprimeB = texture2DRect(BLines, vec2(i, 0.5)).xy;
		vec2 QprimeB = texture2DRect(BLines, vec2(i, 0.5)).zw;
		vec2 XprimeB = calcXPrime(PprimeB, QprimeB, uv);

		vec2 displacementA = XprimeA - X;
		vec2 displacementB = XprimeB - X;

		float dist;
		if(uv.x > 1.0)
			dist = distance(Q, X);
		else if(uv.x < 0.0)
			dist = distance(P, X);
		else
			dist = abs(uv.y);
		float len = distance(P, Q);
		float weight = pow(len, p) / (a + dist);
		weight = pow(weight, b);

		dsumA += displacementA * weight;
		dsumB += displacementB * weight;

		weightsum += weight;
	}

	vec2 XprimeA = X + dsumA / weightsum;
	vec2 XprimeB = X + dsumB / weightsum;

	vec4 startPixel, endPixel;
	if(XprimeA.x >= 0.0 && XprimeA.x < TexWidth && XprimeA.y >= 0.0 && XprimeA.y < TexHeight)
		startPixel = texture2DRect(TexA, XprimeA);
	else
		startPixel = texture2DRect(TexA, gl_FragCoord.xy);

	if(XprimeB.x >= 0.0 && XprimeB.x < TexWidth && XprimeB.y >= 0.0 && XprimeB.y < TexHeight)
		endPixel = texture2DRect(TexB, XprimeB);
	else
		endPixel = texture2DRect(TexB, gl_FragCoord.xy);

	if(abs(BlendType) < Epsilon)
		gl_FragColor = mix(startPixel, endPixel, Step);
	else if(abs(BlendType - 1.0) <= Epsilon)
		gl_FragColor = startPixel;
	else
		gl_FragColor = endPixel;
}