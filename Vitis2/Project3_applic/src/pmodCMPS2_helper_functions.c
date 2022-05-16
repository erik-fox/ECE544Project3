/****************************************************************************/
/*              PmodCMPS2 helper functions (most provided by Digilent)      */
/*                                                                          */
/* These functions were included in Digilent's PmodCMPS2 example.  I made   */
/* some modifications to the functions to make them work in my prototype    */
/* and added a function or two.  You are welcome to use them but caveat     */
/* emptor (buyer be aware) - I do not guarantee that they will work any     */
/* better than the functions provided by Digilent, nor do I guarantee that  */
/* that they will perform as expected in your application.  I am providing  */
/* this file for your convenience.                                          */
/****************************************************************************/

/**
 * Function to clear the compass calibration values
 *
 * (Re)initializes the calibration data structure for the PmodCMPS
 *
 * @param *calib    pointer to the the compass calibration structure
 *
 */
/*
static void CMPS2_ClearCalibration(CMPS2_CalibrationData *calib)
{
    calib->max.x = 0x8000; // Center point of 0x0000 -> 0xFFFF
    calib->max.y = 0x8000;
    calib->max.z = 0x8000;
    calib->min.x = 0x8000;
    calib->min.y = 0x8000;
    calib->min.z = 0x8000;
    calib->mid.x = 0x8000;
    calib->mid.y = 0x8000;
    calib->mid.z = 0x8000;
}
*/
/**
 * Function to update the compass calibration values
 *
 * Tracks/updates the min, max, and average values of the 3 compass axis
 *
 * @param *calib    pointer to the the compass calibration structure
 * @param data      Data packed from the compass
 *
 */
/*
static void CMPS2_Calibrate(CMPS2_CalibrationData *calib, CMPS2_DataPacket data)
{
    if (data.x > calib->max.x) calib->max.x = data.x; // Track maximum / minimum
    if (data.y > calib->max.y) calib->max.y = data.y; // value seen per axis
    if (data.z > calib->max.z) calib->max.z = data.z;
    if (data.x < calib->min.x) calib->min.x = data.x;
    if (data.y < calib->min.y) calib->min.y = data.y;
    if (data.z < calib->min.z) calib->min.z = data.z;
    
    calib->mid.x = (calib->max.x >> 1) + (calib->min.x >> 1); // Find average
    calib->mid.y = (calib->max.y >> 1) + (calib->min.y >> 1);
    calib->mid.z = (calib->max.z >> 1) + (calib->min.z >> 1);
}
*/

/**
 * Function to convert a compass reading into degrees
 *
 * Does the math to convert the compass data to degrees.  Makes use
 * of floating point math so you may probably want to leave it out of
 * an interrupt handler.  You may also want to configure your Microblaze
 * with hardware floating point.
 *
 *
 * @param calib         a compass calibration structure
 * @param data          a data packet from the compass
 * @param declination   offset to the compass reading.  Based on location
 *
 * @return              the compass reading converted to degrees
 *
 */
/*
int CMPS2_ConvertDegree(CMPS2_CalibrationData calib,
        CMPS2_DataPacket data, int declination)
{
    float tx, ty;
    float deg;
//print("starting CMPS2_ConvertDegree\n\r");
    if (data.x < calib.mid.x)
        tx = (calib.mid.x - data.x);
    else
        tx = (data.x - calib.mid.x);

    if (data.y < calib.mid.y)
        ty = (calib.mid.y - data.y);
    else
        ty = (data.y - calib.mid.y);
if (tx == 0.0) {
	print("tx = 0\n\r");
	return -1;
}
    if (data.x < calib.mid.x) {
        if (data.y > calib.mid.y)
            deg = 90.0 - atan2f(ty, tx) * 180.0 / 3.14159;
        else
            deg = 90.0 + atan2f(ty, tx) * 180.0 / 3.14159;
    } else {
        if (data.y < calib.mid.y)
            deg = 270.0 - atan2f(ty, tx) * 180.0 / 3.14159;
        else
            deg = 270.0 + atan2f(ty, tx) * 180.0 / 3.14159;
    }

    deg += declination;

    while (deg >= 360)
        deg -= 360;
    while (deg < 0)
        deg += 360;

    printf("calculating hdg: ty=%f, tx=%f. deg=%f\n\r", ty, tx, deg);
    return (int) deg;
}
*/

/**
 * Function to compare to PmodCMPS2 readings
 *
 * Compares the 3 axis values in a CMPS2_DataPacket and returns
 * -1, 0, or 1 in the same way that compare functions are written
 * for qsort() and bsearch().  The function passes in void pointers
 * which have to be cast to the CMPS2 data packet format.
 *
 *
 * @param obj1          a void pointer to one compass reading
 * @param obj2          a void pointer to the compass reading to compare
 *
 * @return              0 if the readings are identical.  -1 if one of
 *                      the readings is less than the other. +1 if one
 *                      of the readings is greater than the other
 *
 * @note The < and > readings are not really representative of the contents
 * of the readings since we are dealing w/ using unsigned 16-bit variables
 * but all we care about in this application is whether they are equal
 */
/*
static int CMPS2_compare_compass_readings(const void* obj1, const void* obj2)
{
  const CMPS2_DataPacket* d1 = obj1;
  const CMPS2_DataPacket* d2 = obj2;
  int diff;

  diff = d1->x - d2->x;
  if(diff != 0)
    return diff;

  diff = d1->y - d2->y;
  if(diff != 0)
    return diff;

  diff = d1->z - d2->z;
  return diff;
}
*/
