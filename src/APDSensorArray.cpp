/* APDuinOS Library
 * Copyright (C) 2012 by Gy�rgy Schreiber
 *
 * This file is part of the APDuinOS Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the APDuinOS Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * APDSensorArray.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: George Schreiber
 */

#include "APDSensorArray.h"



APDSensorArray::APDSensorArray()
{
  // TODO Auto-generated constructor stub
  pAPDSensors = NULL;
  iSensorCount = 0;
  iNextSensor = 0;
  pfruleeval = NULL;
  pRA = NULL;
}

APDSensorArray::~APDSensorArray()
{
  // TODO Auto-generated destructor stub
  if (this->pAPDSensors != NULL) {
      for (int i=0; i<this->iSensorCount; i++) {
          if (this->pAPDSensors[i] != NULL) {
          	SerPrintP("del sens pos "); Serial.print(i); delay(50);
              delete((this->pAPDSensors[i]));				// each APDSensor was new by 'new_sensor_parser'
              SerPrintP("-done.\n"); delay(20);
              this->pAPDSensors[i] = NULL;
          }
      }
      free(this->pAPDSensors);										// APDSensor pointers array was malloc
      this->pAPDSensors = NULL;
      this->iSensorCount=0;
  }
}

void APDSensorArray::enableRuleEvaluation(void (*pfunc)(void*, APDSensor *),void *pra) {
	this->pfruleeval = pfunc;
	this->pRA = pra;
}


int APDSensorArray::dumpToFile(APDStorage *pAPDStorage, char * pszFileName) {
  // make a string for assembling the data to log:
#ifdef DEBUG
  SerPrintP("Dumping Sensor Array Config...");
#endif
    if (pAPDStorage->p_sd->exists(pszFileName)) {
          pAPDStorage->p_sd->remove(pszFileName);
        }
  SdFile dataFile(pszFileName, O_WRITE | O_CREAT );
  if (dataFile.isOpen()) {
    for (int i=0; i<iSensorCount; i++) {
      char line[BUFSIZ]="";
      APDSensor *ps = pAPDSensors[i];
      // TODO update with recent fields
      sprintf(line,"%s %i,%i,%i,%i,%i,%i,%i,%s",ps->config.label,ps->config.sensor_type,ps->config.sensor_class,ps->config.sensor_subtype,ps->config.sensor_pin,ps->config.sensor_secondary_pin,ps->config.sensor_freq,ps->config.sensor_log,ps->config.extra_data);
      dataFile.println(line);
    }
    dataFile.close();
#ifdef DEBUG
    SerPrintP("Sensor Array Config dumped.");
#endif
  }
  else {
      // TODO add an error macro in storage, replace all error opening stuff with reference to that
    SerPrintP("E305 ('"); Serial.print(pszFileName); SerPrintP("')\n");			// E305 - error opening dump file
  }
}




void APDSensorArray::new_sensor_parser(void *pSA, int iline, char *psz) {
  SDCONF sdc;
  memcpy(&sdc,0,sizeof(SDCONF));
  char szExtra[24] = "";
#ifdef DEBUG
  SerPrintP("\nSDCONF READS: \""); Serial.print(psz); SerPrintP("\"");
#endif
  int iscand = sscanf( psz, "%s %d,%d,%d,%d,%d,%d,%d,%s",
      (sdc.label),
      &(sdc.sensor_type),
      &(sdc.sensor_class),
      &(sdc.sensor_subtype),
      &(sdc.sensor_pin),
      &(sdc.sensor_secondary_pin),
      &(sdc.sensor_freq),
      &(sdc.sensor_log),
      (sdc.extra_data));

  if (iscand < 9) {
#ifdef DEBUG
      SerPrintP("no extra data");
#endif
      memset(sdc.extra_data,0,sizeof(char)*24);
  }
#ifdef DEBUG
  SerPrintP("CONF SCANNED: ");
  Serial.print(sdc.label); SerPrintP(",");
  Serial.print(sdc.sensor_type); SerPrintP(",");
  Serial.print(sdc.sensor_class); SerPrintP(",");
  Serial.print(sdc.sensor_subtype); SerPrintP(",");
  Serial.print(sdc.sensor_pin); SerPrintP(",");
  Serial.print(sdc.sensor_secondary_pin); SerPrintP(",");
  Serial.print(sdc.sensor_freq); SerPrintP(",");
  Serial.print(sdc.sensor_log); SerPrintP(",");
  Serial.print(sdc.extra_data); SerPrintP("\n");
#endif
  //TODO add compatibility code (so other options can follow, even if not parsed)

  APDSensor *newsensor = NULL;
  APDSensor *reusablesensor = ((APDSensorArray *)pSA)->findReusableSensor(&sdc);

  //(((APDuino *)pAPD)->pAPDSensors[iline]) = new APDSensor(&sdc);
  switch (sdc.sensor_type) {
  case ANALOG_SENSOR:
    SerPrintP("ANALOG");
    newsensor = new AnalogSensor(&sdc);
    break;
  case DIGITAL_SENSOR:
    SerPrintP("DIGITAL");
    newsensor = new DigitalSensor(&sdc);
    break;
  case DHT_SENSOR:
    SerPrintP("DHT");
    newsensor = new DHTSensor(&sdc, reusablesensor);
    break;
  case ONEWIRE_SENSOR:
    SerPrintP("1-WIRE");
    newsensor = new OneWireSensor(&sdc, reusablesensor);
    break;
  case SONAR_SENSOR:
    SerPrintP("SONAR");
    newsensor = new SonarSensor(&sdc);
    break;
  case BMP085_SENSOR:
    SerPrintP("BMP085");
    newsensor = new BMPSensor(&sdc, reusablesensor);
    break;
  case VIBRATION_SENSOR:
    SerPrintP("VIBRATION");
    newsensor = new VibrationSensor(&sdc);
    break;
  case ATLASSCIENTIFIC_SENSOR:
      SerPrintP("ATLASSCIENTIFIC");
      newsensor = new AtlasScientificSensor(&sdc, reusablesensor);
      break;
  default:
    SerPrintP("E310");			// E310 - unknown sensor type
  }
  ((APDSensorArray*)pSA)->pAPDSensors[iline] = newsensor;
  // done with sensor definition line
}

APDSensor *APDSensorArray::firstSensorByPin(int iPin, int iType) {
  APDSensor *psens = NULL;
  for (int i=0; i<this->iSensorCount && this->pAPDSensors[i] != NULL; i++) {
      if (this->pAPDSensors[i]->config.sensor_type == iType &&
          this->pAPDSensors[i]->config.sensor_pin == iPin) {
          psens = this->pAPDSensors[i];
          break;
      }
  }
  return psens;
}

int APDSensorArray::indexBySensor(APDSensor *pSensor) {
	int idx = -2;
  if (pSensor != NULL) {
  	idx = -1;
		for (int i=0; i<this->iSensorCount && this->pAPDSensors[i] != NULL; i++) {
				if (this->pAPDSensors[i] == pSensor) {
						idx = i;
						break;
				}
		}
  }
  return idx;
}


APDSensor *APDSensorArray::findReusableSensor(SDCONF *sdc) {
  APDSensor *preusable = NULL;
  switch (sdc->sensor_type) {
  // TODO: put all reusable sensors below
  case ONEWIRE_SENSOR:
  case DHT_SENSOR:
  case BMP085_SENSOR:
  case ATLASSCIENTIFIC_SENSOR:
    preusable = this->firstSensorByPin(sdc->sensor_pin, sdc->sensor_type);
    if (preusable) {
        SerPrintP("REUSING @");
        Serial.print((unsigned int)preusable,HEX);
    }
    break;
  default:
    SerPrintP("NOT ");
  }
  SerPrintP("REUSABLE");
  return preusable;
}

APDSensor *APDSensorArray::byIndex(int idx) {
	APDSensor *ret = NULL;
  if (idx >= 0 && idx <= this->iSensorCount) {
  	ret = this->pAPDSensors[idx];
  }
  return ret;
}




int APDSensorArray::loadSensors(APDStorage *pAPDStorage) {
  if (pAPDSensors == NULL) {    // if no sensor array
#ifdef DEBUG
    SerPrintP("Counting sensors...");
#endif
    // TODO check if SD is available!
    iSensorCount =  get_line_count_from_file("SENSORS.CFG");
    Serial.print(iSensorCount); SerPrintP(" sensors seem to be defined...");

    if (iSensorCount > -1) {
      SerPrintP("Sensor Array: allocating "); Serial.print(sizeof(APDSensor *)*iSensorCount,DEC); SerPrintP(" bytes of RAM\n");

      pAPDSensors = (APDSensor**)malloc(sizeof(APDSensor*)*iSensorCount);

      if (pAPDSensors != NULL) {
        memset(pAPDSensors,0,sizeof(APDSensor*)*iSensorCount);

#ifdef DEBUG
        SerPrintP("Sensor Array allocated. Populating from SENSORS.CFG...");
#endif

        pAPDStorage->readFileWithParser((char *)"SENSORS.CFG",&new_sensor_parser, (void*)this);
#ifdef DEBUG
        SerPrintP("We should be done with the sensors.\n");
#endif
        // TODO add any postprocessing

        iNextSensor = 0;                // first sensor to read
      } else {
        SerPrintP("E303\n");					// failed to allocate array
      }
    } else {
        SerPrintP("E302\n");					// no sensors
    }
  } else {
    SerPrintP("E301\n");							// already there
  }
  return iSensorCount;
}




int APDSensorArray::count() {
  return this->iSensorCount;
}

char *APDSensorArray::labelS(int iSensorIdx, char *szlabel) {
  //SerPrintP("labelS\n"); Serial.print(iSensorIdx);
  strcpy(szlabel,"");
  if (iSensorIdx >= 0 && iSensorIdx < this->iSensorCount) {
#ifdef DEBUG
      SerPrintP("calling getlabel"); Serial.println(iSensorIdx);
#endif
      strncpy(szlabel,this->pAPDSensors[iSensorIdx]->config.label,12);  // FIXME setup DEFAULT label len in header
#ifdef DEBUG
      Serial.println(szlabel);
#endif
  }
  return szlabel;
}

char *APDSensorArray::valueS(int iSensorIdx, char *szvalue) {
  //SerPrintP("valueS\n"); Serial.print(iSensorIdx);
  strcpy(szvalue,"");
  if (iSensorIdx >= 0 && iSensorIdx < this->iSensorCount) {
#ifdef DEBUG
    SerPrintP("calling getval"); Serial.println(iSensorIdx);
#endif
    this->pAPDSensors[iSensorIdx]->getValueS(szvalue);
#ifdef DEBUG
    Serial.println(szvalue);
#endif
  }
  return szvalue;
}





void APDSensorArray::pollSensors(boolean bProcessRules) {
#ifdef DEBUG
  SerPrintP("POLL APD SENSOR..."); Serial.print(iNextSensor); SerPrintP("...");
  Serial.print(iSensorCount); SerPrintP(" SENSORS IN TOTAL, CHECKING "); Serial.println(iNextSensor);
  delay(10);
#endif
  if (iNextSensor >= 0 && iNextSensor < this->iSensorCount && pAPDSensors != NULL)                // check if we have sensors
    if (pAPDSensors[iNextSensor]->check()) {
    	if (bProcessRules == true && this->pfruleeval != NULL && this->pRA != NULL) {
#ifdef DEBUG
    		SerPrintP("early eval");
#endif
    		(*this->pfruleeval)(this->pRA,pAPDSensors[iNextSensor]);
      }
    }
  iNextSensor = iNextSensor + 1 < this->iSensorCount ? iNextSensor + 1 : 0;    // set next sensor
  //iNextSensor = (iNextSensor + 1) % this->iSensorCount;    // set next sensor
}



void APDSensorArray::diagnostics() {
  for (int i=0; i<this->iSensorCount; i++) {
      this->pAPDSensors[i]->diagnostics();
  }
}
