/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RestApi.h
 * Author: lubos
 *
 * Created on 28. března 2018, 0:15
 */

#ifndef RESTAPI_H
#define RESTAPI_H
#include "web/WebRequest.h"
#include "web/WebResponse.h"
#include "web/WebRouter.h"
#include "FileManager.h"
#include "PrinterManager.h"

void routePrinter(WebRouter* router, FileManager& fileManager, PrinterManager& printerManager);

#endif /* RESTAPI_H */

