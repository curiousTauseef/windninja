/******************************************************************************
 *
 * $Id$
 *
 * Project:  WindNinja
 * Purpose:  A class for storing point weather station data
 * Author:   Jason Forthofer <jforthofer@gmail.com>
 *
 ******************************************************************************
 *
 * THIS SOFTWARE WAS DEVELOPED AT THE ROCKY MOUNTAIN RESEARCH STATION (RMRS)
 * MISSOULA FIRE SCIENCES LABORATORY BY EMPLOYEES OF THE FEDERAL GOVERNMENT
 * IN THE COURSE OF THEIR OFFICIAL DUTIES. PURSUANT TO TITLE 17 SECTION 105
 * OF THE UNITED STATES CODE, THIS SOFTWARE IS NOT SUBJECT TO COPYRIGHT
 * PROTECTION AND IS IN THE PUBLIC DOMAIN. RMRS MISSOULA FIRE SCIENCES
 * LABORATORY ASSUMES NO RESPONSIBILITY WHATSOEVER FOR ITS USE BY OTHER
 * PARTIES,  AND MAKES NO GUARANTEES, EXPRESSED OR IMPLIED, ABOUT ITS QUALITY,
 * RELIABILITY, OR ANY OTHER CHARACTERISTIC.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/
#include "wxStation.h"

wxStation::eStationFormat wxStation::stationFormat = invalidFormat;  

wxStation::wxStation()
{
    initialize();
}

/**
 * Copy Constructor for wxStation
 * @param m right hand side wxStation object
 */
wxStation::wxStation( wxStation const& m )
{
    stationName = m.stationName;
    lat = m.lat;
    lon = m.lon;
    projXord = m.projXord;
    projYord = m.projYord;
    xord = m.xord;
    yord = m.yord;
    heightList = m.heightList;
    heightUnits = m.heightUnits;
    speedList = m.speedList;
    inputSpeedUnits = m.inputSpeedUnits;
    directionList = m.directionList;
    w_speed = m.w_speed;
    w_speedUnits = m.w_speedUnits;
    temperatureList = m.temperatureList;
    tempUnits = m.tempUnits;
    cloudCoverList = m.cloudCoverList;
    cloudCoverUnits = m.cloudCoverUnits;
    influenceRadiusList = m.influenceRadiusList;
    influenceRadiusUnits = m.influenceRadiusUnits;
    datumType = m.datumType;
    coordType = m.coordType;
    datetimeList = m.datetimeList;
    localDateTimeList=m.localDateTimeList;
    curStep=m.curStep;

}

/**
 * Equals operator for wxStation
 * @param m right hand side wxStation object
 * @return reference to a new wxStation object
 */
wxStation& wxStation::operator= ( wxStation const& m )
{
    if( &m != this ) {
        stationName = m.stationName;
        lat = m.lat;
        lon = m.lon;
        projXord = m.projXord;
        projYord = m.projYord;
        xord = m.xord;
        yord = m.yord;
        heightList = m.heightList;
        heightUnits = m.heightUnits;
        speedList = m.speedList;
        inputSpeedUnits = m.inputSpeedUnits;
        directionList = m.directionList;
        w_speed = m.w_speed;
        w_speedUnits = m.w_speedUnits;
        temperatureList = m.temperatureList;
        tempUnits = m.tempUnits;
        cloudCoverList = m.cloudCoverList;
        cloudCoverUnits = m.cloudCoverUnits;
        influenceRadiusList = m.influenceRadiusList;
        influenceRadiusUnits = m.influenceRadiusUnits;
        datumType = m.datumType;
        coordType = m.coordType;
        datetimeList =m.datetimeList;
        localDateTimeList=m.localDateTimeList;
        curStep=m.curStep;
    }
    return *this;
}

wxStation::~wxStation()
{

}

/**
 * Initialize a wxStation object with default values
 */
void wxStation::initialize()
{
    stationName = "";
    lat = lon = 0.0;
    datumType = wxStation::WGS84;
    coordType = wxStation::GEOGCS;
    heightUnits = lengthUnits::meters;
    inputSpeedUnits = velocityUnits::metersPerSecond;
    w_speed = 0.0;
    w_speedUnits = velocityUnits::metersPerSecond;
    tempUnits = temperatureUnits::K;
    cloudCoverUnits = coverUnits::fraction;
    influenceRadiusUnits = lengthUnits::meters;
    datumType = WGS84;
    coordType = GEOGCS;
}

/**
 * Fetch the height of the wxStation.  Default height is meters
 * @param units to retrieve the height in
 * @return height of station
 */
double wxStation::get_height(lengthUnits::eLengthUnits units)
{
    double h;
    lengthUnits::fromBaseUnits( h, units );
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int i=0;i<datetimeList.size();i++)
    {
        if (cur==localDateTimeList[i])
        {
            h=heightList[i];
            break;
        }
    }

    return h;
}

/**
 * Fetch the station speed, default unit is mps
 * @param units to retrieve speed in
 * @return speed
 */
double wxStation::get_speed(velocityUnits::eVelocityUnits units)
{
    double s;
    velocityUnits::fromBaseUnits( s, units );
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int i=0;i<datetimeList.size();i++)
    {
        if (cur==localDateTimeList[i])
        {
            s=speedList[i];
            break;
        }
    }

    return s;
}

double wxStation::get_direction()
{
    double d;
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int i=0;i<datetimeList.size();i++)
    {
        if (cur==localDateTimeList[i])
        {
            d=directionList[i];
            break;
        }
    }

    return d;
}

/**
 * Fetch the vertical speed for the station
 * @param units to retrieved the vertical speed in, default is mps
 * @return vertical speed
 */
double wxStation::get_w_speed( velocityUnits::eVelocityUnits units)
{
    double w = w_speed;
    velocityUnits::fromBaseUnits( w, units );
    return w;
}

/**
 * Fetch temperature for the station
 * @param units to retrieve the temperature in, default is K
 * @return temperature
 */
double wxStation::get_temperature(temperatureUnits::eTempUnits units)
{
    double t;
    temperatureUnits::fromBaseUnits( t, units );
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int i=0;i<datetimeList.size();i++)
    {
        if (cur==localDateTimeList[i])
        {
            t=temperatureList[i];
            break;
        }
    }

    return t;
}

/**
 * Fetch the cloud cover for the station
 * @param units to retrieve the cover in, default is fraction
 * @return cloud cover
 */
double wxStation::get_cloudCover(coverUnits::eCoverUnits units)
{
    double c;
    coverUnits::fromBaseUnits( c, units );
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int i=0;i<datetimeList.size();i++)
    {
        if (cur==localDateTimeList[i])
        {
            c=cloudCoverList[i];
            break;
        }
    }

    return c;
}

/**
 * Fetch the radius of influence of the station
 * @param units to retrieve the radius in, default is meters
 * @return radius of infuence
 */
double wxStation::get_influenceRadius(lengthUnits::eLengthUnits units)
{
    double i;
    lengthUnits::fromBaseUnits( i, units );
    boost::local_time::local_date_time cur=get_currentTimeStep();

    for (int k=0;k<datetimeList.size();k++)
    {
        if (cur==localDateTimeList[k])
        {
            i=influenceRadiusList[k];
            break;
        }
    }

    return i;
}

/**
 * Set the weather station name
 * @param Name representation of the station name.
 */
void wxStation::set_stationName( std::string Name )
{
    stationName = Name;
}

/**
 * Set the location of the station in lat/lon based on projected coordinates
 * @param Xord x coordinate in system
 * @param Yord y coordinate in system
 * @param demFile name of the dem file to create the coordinate transformation
 * from
 */
void wxStation::set_location_projected( double Xord, double Yord,
                    std::string demFile )
{
    projXord = Xord;
    projYord = Yord;

    if( demFile.empty() || demFile == "" )
        return;

    //get llcorner to subtract
    GDALDataset *poDS = (GDALDataset*) GDALOpen( demFile.c_str(), GA_ReadOnly );
    if( poDS == NULL ){
        xord = Xord;
        yord = Yord;
        return;
    }

    double adfGeoTransform[6];

    if( poDS->GetGeoTransform( adfGeoTransform ) != CE_None ) {
        xord = Xord;
        yord = Yord;
        return;
    }

    double llx = 0.0;
    double lly = 0.0;
    llx = adfGeoTransform[0];
    lly = adfGeoTransform[3] + ( adfGeoTransform[5] * poDS->GetRasterYSize() );

    xord = Xord - llx;
    yord = Yord - lly;

    double lonx = projXord;
    double laty = projYord;

    GDALPointToLatLon( lonx, laty, poDS, "WGS84" );
    datumType = WGS84;
    coordType = PROJCS;

    lon = lonx;
    lat = laty;

    GDALClose( (GDALDatasetH)poDS );
}

/**
 * Set location of the station based on latitude and longitude.  Datum
 * transformation info may need to occur
 * @param Lat latitude of the station
 * @param Lon longitude of the station
 * @param demFile file name of the input dem, for coord tranformation
 * @param datum datum to convert to.
 */
void wxStation::set_location_LatLong( double Lat, double Lon,
                                      const std::string demFile,
                      const char *datum )
{
    lon = Lon;
    lat = Lat;

    if( demFile.empty() || demFile == "" )
        return;

    GDALDataset *poDS = (GDALDataset*)GDALOpen( demFile.c_str(), GA_ReadOnly );

    if( poDS == NULL )
        return;

    double projX = lon;
    double projY = lat;

    GDALPointFromLatLon( projX, projY, poDS, datum );

    projXord = projX;
    projYord = projY;

    //still assume dem is projected.
    double llx = 0.0;
    double lly = 0.0;
    double adfGeoTransform[6];

    if( poDS->GetGeoTransform( adfGeoTransform ) != CE_None )
    {
        xord = projXord;
        yord = projYord;
        return;
    }

    llx = adfGeoTransform[0];
    lly = adfGeoTransform[3] + ( adfGeoTransform[5] * poDS->GetRasterYSize() );

    xord = projXord - llx;
    yord = projYord - lly;

    if( EQUAL( datum, "WGS84" ) )
        datumType = WGS84;
    else if( EQUAL( datum, "NAD83" ) )
        datumType = NAD83;
    else if ( EQUAL( datum, "NAD27" ) )
        datumType = NAD27;

    coordType = GEOGCS;

    GDALClose( (GDALDatasetH)poDS );
}

/**
 * Set the height of the station
 * @param Height height of the station
 * @param units units of the Height parameter
 * @see ninjaUnits
 */
void wxStation::set_height( double Height, lengthUnits::eLengthUnits units )
{
    lengthUnits::toBaseUnits( Height, units );
    heightList.push_back(Height);
    heightUnits = units;
}

/** Set the speed for the station
 *
 *
 * @param Speed
 * @param units
 */
void wxStation::set_speed( double Speed, velocityUnits::eVelocityUnits units )
{
    velocityUnits::toBaseUnits( Speed, units );
    speedList.push_back(Speed);
    inputSpeedUnits = units;
}

void wxStation::set_direction( double Direction )
{
    directionList.push_back(Direction);
}

void wxStation::set_w_speed( double W_Speed, velocityUnits::eVelocityUnits units )
{
    velocityUnits::toBaseUnits( W_Speed, units );
    w_speed = W_Speed;
    w_speedUnits = units;
}

void wxStation::set_temperature( double Temperature,
                 temperatureUnits::eTempUnits units )
{
    temperatureUnits::toBaseUnits( Temperature, units );
    temperatureList.push_back(Temperature);
    tempUnits = units;
}

void wxStation::set_cloudCover( double CloudCover,
                coverUnits::eCoverUnits units )
{
    coverUnits::toBaseUnits( CloudCover, units );
    cloudCoverList.push_back(CloudCover);
    cloudCoverUnits = units;
}

void wxStation::set_influenceRadius( double InfluenceRadius,
                     lengthUnits::eLengthUnits units )
{
    lengthUnits::toBaseUnits( InfluenceRadius, units );
    influenceRadiusList.push_back(InfluenceRadius);
    influenceRadiusUnits = units;
}

void wxStation::set_datetime(boost::posix_time::ptime timedata)
{
    datetimeList.push_back(timedata);
}

boost::posix_time::ptime wxStation::get_datetime(int idx)
{
    boost::posix_time::ptime time=datetimeList[idx];
    return time;
}

void wxStation::set_localDateTime(boost::local_time::local_date_time timedata)
{
    localDateTimeList.push_back(timedata);
}

void wxStation::update_direction(double Direction, int index)// Used for Match Points
{
    directionList[index]=Direction;
}

void wxStation::update_speed(double Speed, velocityUnits::eVelocityUnits units, int index)// Used for Match Points
{
    velocityUnits::toBaseUnits( Speed, units );
    speedList[index]=Speed;
    inputSpeedUnits = units;
}

int wxStation::get_listSize()
{
    int size=speedList.size();
    return size;
}

boost::local_time::local_date_time wxStation::get_localDateTime(int idx)
{
    boost::local_time::local_date_time time=localDateTimeList[idx];
    return time;
}

boost::local_time::local_date_time wxStation::get_currentTimeStep()
{
    boost::local_time::local_date_time current=curStep[0];

    return current;
}

void wxStation::set_currentTimeStep(boost::local_time::local_date_time step)
{
    curStep.assign(1,step);
}

bool wxStation::check_station(wxStation station)
{
    if(station.stationName.empty() || station.stationName == "")
    {
        cout<<"failed Name Check"<<endl;
        return false;
    }
    if(station.coordType != GEOGCS && station.coordType != PROJCS)
    {
        cout<<"failed Coord Check"<<endl;
        return false;
    }
    if(station.datumType != WGS84 && station.datumType != NAD83 && station.datumType !=NAD27)
    {
        cout<<"failed datum Check"<<endl;
        return false;
    }
    if(station.lat < -90.0 || station.lat > 90.0)
    {
        cout<<"failed lat Check"<<endl;
        return false;
    }
    if(station.lon < -180.0 || station.lon > 360.0)
    {
        cout<<"failed lon Check"<<endl;
        return false;
    }

    for (int i=0;i<station.heightList.size();i++)
    {
        if(station.heightList[i] < 0.0)
        {
            cout<<"failed height Check on "<<i<<endl;
            cout<<station.heightList[i]<<endl;
            return false;
        }
        if(station.speedList[i] < 0.0)
        {
            cout<<"failed speed Check on "<<i<<endl;
            cout<<station.speedList[i]<<endl;
            return false;
        }
        if(station.directionList[i] < 0.0 || station.directionList[i] > 360.0)
        {
            cout<<"failed direction Check on "<<i<<endl;
            cout<<station.directionList[i]<<endl;
            return false;
        }
        if(station.temperatureList[i]< 173.15 || station.temperatureList[i] > 330.00)
        {
            cout<<"failed temperature Check on "<<i<<endl;
            cout<<station.temperatureList[i]<<endl;
            return false;
        }
        if(station.cloudCoverList[i]<0.0||station.cloudCoverList[i]>1.10)
        {
            cout<<"failed cloud check on "<<i<<endl;
            cout<<station.cloudCoverList[i]<<endl;
            return false;
        }
    }

    if(station.w_speed < 0.0)
    {
        cout<<"failed vert_speed Check"<<endl;
        return false;
    }

    return true;
}

/**
 * Write CSV and KML files and stuff used to be here, I don't know what do to/if they are needed
 */

void wxStation::stationViewer(wxStation station)
{

}

void wxStation::SetStationFormat(eStationFormat format)
{
    stationFormat = format;
}

wxStation::eStationFormat wxStation::GetStationFormat()
{
    return stationFormat;
}

/**
 * Check the validity of the csv file named pszFilename.  If it is valid, return
 * the format version of the data.
 *
 * 1 -> original windninja format, in use until 3.2.0 (TODO(kyle):update
 * version)
 * 2 -> format after implemententing the station fetch functionality.
 *
 * If the header is not valid, return -1;
 *
 * \param pszFilename the path of the file to test
 *
 * \return -1 if the file is invalid in anyway, or the version of the file(1 or
 * 2).
 */
int wxStation::GetHeaderVersion(const char *pszFilename)
{
    OGRDataSourceH hDS = NULL;
    hDS = OGROpen(pszFilename, FALSE, NULL);
    int rc = 0;
    if (hDS == NULL) {
        return -1;
    }
    OGRLayerH hLayer = NULL;
    hLayer = OGR_DS_GetLayer(hDS, 0);
    if (hLayer == NULL) {
        rc = -1;
    }
    OGRFeatureDefnH hDefn = NULL;
    hDefn = OGR_L_GetLayerDefn(hLayer);
    if (hDefn == NULL) {
        rc = -1;
    }
    // If we failed to open or get metadata, bail
    if (rc == -1) {
        OGR_DS_Destroy(hDS);
        return -1;
    }

    /*
    ** Iterate through the expected columns.  If the layer doesn't runs out of
    ** fields before we get to the end of the expected header, GetFieldDefn() will
    ** return NULL, so there is no explicit count check needed.
    */

    int n = CSLCount((char **)apszValidHeader1);
    /* We'll use our index later to check for more fields */
    int i = 0;

    assert(rc == 0);
    OGRFieldDefnH hFldDefn = NULL;
    for (i = 0; i < n; i++) {
        hFldDefn = OGR_FD_GetFieldDefn(hDefn, i);
        if (hFldDefn == NULL) {
            rc = -1;
        }
        if (!EQUAL(OGR_Fld_GetNameRef(hFldDefn), apszValidHeader1[i])) {
            rc = -1;
        }
    }

    // If we failed to get version 1 columns, bail
    if (rc == -1) {
        OGR_DS_Destroy(hDS);
        return -1;
    }

    /*
    ** Now we have a valid header for version 1.  If there are more fields, we
    ** check them.  If not, we are done.
    **
    ** TODO(kyle): should we accept a version 1 file with extra non-valid fields?
    */
    rc = 1;
    if (OGR_FD_GetFieldCount(hDefn) > n) {
        if (!EQUAL(OGR_Fld_GetNameRef(hFldDefn), apszValidHeader2[i])) {
            /* If we silently except version 1 files, return 1 here. */
            rc = -1;
        }
        rc = 2;
    }
    OGR_DS_Destroy(hDS);
    return rc;
}

/**Write a csv file with no data, just a header
 * @param outFileName file to write
 */
void wxStation::writeBlankStationFile(std::string outFileName)
{
    if (outFileName.empty() || outFileName == "") {
        return;
    }
    FILE *fout;
    fout = fopen(outFileName.c_str(), "w");
    if (fout == NULL) {
        return;
    }
    int n = CSLCount( (char**)apszValidHeader2 );
    for (int i = 0; i < n - 1; i++) {
        fprintf(fout, "\"%s\",", apszValidHeader2[i]);
    }
    fprintf(fout, "\"%s\"\n", apszValidHeader2[n - 1]);
    fclose(fout);
}

/** Write a csv file representing interpolated wxStation data
 * @param StationVect std::vector of wxStation objects
 * @param outFileName output file name as std::string
 */
void wxStation::writeStationFile( std::vector<wxStation>StationVect,
                                  std::string outFileName )
{
    std::string outFileNameStamp;
    std::string outFileNameMod;
    boost::local_time::local_date_time stamp=StationVect[0].get_currentTimeStep();

    boost::local_time::local_time_facet* timeOutputFacet;
    timeOutputFacet = new boost::local_time::local_time_facet();
    std::ostringstream timestream;
    timestream.imbue(std::locale(std::locale::classic(), timeOutputFacet));
    timeOutputFacet->format("%m-%d-%Y_%H%M");

    timestream<<stamp;

    if (outFileName.substr(outFileName.size()-4,4)==".csv")
    {
        outFileNameMod=outFileName.substr(0,outFileName.size()-4);
    }
    else
    {
        outFileNameMod=outFileName;
    }
    outFileNameStamp=outFileNameMod+"-"+timestream.str()+".csv";

    if( outFileNameStamp.empty() || outFileNameStamp == "" )
        return;
    if( StationVect.empty() ) {
        writeBlankStationFile( outFileNameStamp );
        return;
    }

    FILE *fout;
    fout = fopen( outFileNameStamp.c_str(), "w" );
    if (fout == NULL) {
        return;
    }
    int n = CSLCount((char **)apszValidHeader2);
    for (int i = 0; i < n - 1; i++) {
        fprintf(fout, "\"%s\",", apszValidHeader2[i]);
    }
    fprintf(fout, "\"%s\"\n", apszValidHeader2[n - 1]);

    for( unsigned int i = 0;i < StationVect.size();i++ ) {
        fprintf( fout, "\"%s\",", StationVect[i].get_stationName().c_str() );
        fprintf( fout, "\"GEOGCS\"," );
        fprintf( fout, "\"WGS84\"," );
        fprintf( fout, "%.10lf,", StationVect[i].get_lat() );
        fprintf( fout, "%.10lf,", StationVect[i].get_lon() );
        fprintf( fout, "%.2lf,", StationVect[i].get_height() );
        fprintf( fout, "\"meters\"," );
        fprintf( fout, "%.2lf,", StationVect[i].get_speed() );
        fprintf( fout, "\"mps\"," );
        fprintf( fout, "%.1lf,", StationVect[i].get_direction() );
        fprintf( fout, "%.1lf,", StationVect[i].get_temperature() );
        fprintf( fout, "\"K\"," );
        fprintf( fout, "%.1lf,", StationVect[i].get_cloudCover() * 100 );
        if( StationVect[i].get_influenceRadius() < 0.0 )
            fprintf( fout, "%.2lf,", -1.0 );
        else
            fprintf( fout, "%.2lf,", StationVect[i].get_influenceRadius() );
        fprintf( fout, "\"meters\"," );
        fprintf( fout, "\"%s\",", timestream.str().c_str() );

        fprintf( fout, "\n" );
    }

    fclose( fout );
}

/** Write a point kml file for all the weather stations in stations vector.
 * @param stations A vector of wxStationList objects
 * @param outFileName A string for output file
 * @return void
 */
void wxStation::writeKmlFile( std::vector<wxStation> stations,
        const std::string outFileName )
{
    std::string outFileNameStamp;
    std::string outFileNameMod;
    boost::local_time::local_date_time stamp=stations[0].get_currentTimeStep();

    boost::local_time::local_time_facet* timeOutputFacet;
    timeOutputFacet = new boost::local_time::local_time_facet();
    std::ostringstream timestream;
    timestream.imbue(std::locale(std::locale::classic(), timeOutputFacet));
    timeOutputFacet->format("%m-%d-%Y_%H%M");

    timestream<<stamp;

    if (outFileName.substr(outFileName.size()-4,4)==".kml")
    {
        outFileNameMod=outFileName.substr(0,outFileName.size()-4);
    }
    else
    {
        outFileNameMod=outFileName;
    }

    outFileNameStamp=outFileNameMod+"-"+timestream.str()+".kml";

    if( stations.size() == 0 )
        return;

    FILE *fout = fopen( outFileNameStamp.c_str(), "w" );

    if( fout == NULL )
    {
        printf( "Cannot open kml file: %s for writing.", outFileNameStamp.c_str() );
        return;
    }

    double heightTemp, speedTemp, directionTemp, ccTemp, temperatureTemp, radOfInflTemp;

    fprintf( fout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    fprintf( fout, "<kml>\n" );
    fprintf( fout, "  <Document>\n" );
    fprintf( fout, "    <description>Weather Stations:%s</description>\n",
            outFileNameStamp.c_str() );

    for( unsigned int i = 0;i < stations.size();i++ )
    {
        heightTemp = stations[i].get_height();
        speedTemp = stations[i].get_speed();
        directionTemp = stations[i].get_direction();
        ccTemp = stations[i].get_cloudCover();
        temperatureTemp = stations[i].get_temperature();
        radOfInflTemp = stations[i].get_influenceRadius();

        lengthUnits::fromBaseUnits(heightTemp, stations[i].heightUnits);
        //lengthUnits::fromBaseUnits(heightTemp, lengthUnits::feet);
        velocityUnits::fromBaseUnits(speedTemp, stations[i].inputSpeedUnits);
        //lengthUnits::fromBaseUnits(directionTemp, heightUnits);
        coverUnits::fromBaseUnits(ccTemp, stations[i].cloudCoverUnits);
        temperatureUnits::fromBaseUnits(temperatureTemp, stations[i].tempUnits);
        lengthUnits::fromBaseUnits(radOfInflTemp, stations[i].influenceRadiusUnits);

        fprintf( fout, "    <Placemark>\n" );
        fprintf( fout, "      <name>%s</name\n>",
                stations[i].get_stationName().c_str() );
        fprintf( fout, "    <Point>\n" );
        fprintf( fout, "        <altitudeMode>clampToGround</altitudeMode>\n" );
        fprintf( fout, "        <coordinates>\n" );
        fprintf( fout, "          %lf,%lf,0.0\n", stations[i].get_lon(),
                stations[i].get_lat() );
        fprintf( fout, "        </coordinates>\n" );
        fprintf( fout, "      </Point>\n" );
        fprintf( fout, "      <description>\n" );
        fprintf( fout, "        <![CDATA[\n" );
        fprintf( fout, "          Name: %s\n",
                stations[i].get_stationName().c_str() );
        fprintf( fout, "          Height: %.2lf %s\n", heightTemp,
                lengthUnits::getString(stations[i].heightUnits).c_str() );
        fprintf( fout, "          Speed: %.2lf %s\n",
                speedTemp, velocityUnits::getString(stations[i].inputSpeedUnits).c_str() );
        fprintf( fout, "          Direction: %.0lf %s\n",
                directionTemp, "degrees" );
        fprintf( fout, "          Cloud Cover: %.2lf %s\n",
                ccTemp, coverUnits::getString(stations[i].cloudCoverUnits).c_str() );
        fprintf( fout, "          Temperature: %.1lf %s\n",
                temperatureTemp, temperatureUnits::getString(stations[i].tempUnits).c_str() );

        if(stations[i].get_influenceRadius() > 0.0)
        {
            fprintf( fout, "          Radius of Influence: %.2lf %s\n",
                    radOfInflTemp, lengthUnits::getString(stations[i].influenceRadiusUnits).c_str() );
        }
        else
        {
            fprintf( fout, "          Radius of Influence: infinite\n");
        }
        fprintf( fout, "        ]]>\n" );
        fprintf( fout, "      </description>\n" );
        fprintf( fout, "    </Placemark>\n" );
    }

    fprintf( fout, "  </Document>\n" );
    fprintf( fout, "</kml>\n" );

    fclose( fout );
}

