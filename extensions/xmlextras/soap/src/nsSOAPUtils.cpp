/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsSOAPUtils.h"
#include "nsIDOMText.h"
#include "nsCOMPtr.h"
#include "nsIJSContextStack.h"
#include "nsISOAPParameter.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"

const char* nsSOAPUtils::kSOAPEnvURI = "http://schemas.xmlsoap.org/soap/envelope/";
const char* nsSOAPUtils::kSOAPEncodingURI = "http://schemas.xmlsoap.org/soap/encoding/";
const char* nsSOAPUtils::kEncodingStyleAttribute = "encodingStyle";
const char* nsSOAPUtils::kEnvelopeTagName = "Envelope";
const char* nsSOAPUtils::kHeaderTagName = "Header";
const char* nsSOAPUtils::kBodyTagName = "Body";
const char* nsSOAPUtils::kFaultTagName = "Fault";

void
nsSOAPUtils::GetFirstChildElement(nsIDOMElement* aParent, 
                                  nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  *aElement = nsnull;
  aParent->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      child->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    temp->GetNextSibling(getter_AddRefs(child));
  }
}

void
nsSOAPUtils::GetNextSiblingElement(nsIDOMElement* aStart, 
                                   nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> sibling;

  *aElement = nsnull;
  aStart->GetNextSibling(getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      sibling->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = sibling;
    temp->GetNextSibling(getter_AddRefs(sibling));
  }
}

void 
nsSOAPUtils::GetElementTextContent(nsIDOMElement* aElement, 
                                   nsString& aText)
{
  nsCOMPtr<nsIDOMNode> sibling;
  
  aText.Truncate();
  aElement->GetNextSibling(getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::TEXT_NODE == type) {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(sibling);
      nsAutoString data;
      text->GetData(data);
      aText.Append(data);
    }
    nsCOMPtr<nsIDOMNode> temp = sibling;
    temp->GetNextSibling(getter_AddRefs(sibling));
  }
}

void
nsSOAPUtils::GetInheritedEncodingStyle(nsIDOMElement* aEntry, 
                                       char** aEncodingStyle)
{
  nsCOMPtr<nsIDOMNode> node = aEntry;

  *aEncodingStyle = nsnull;
  while (node) {
    nsAutoString value;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
    if (element) {
      element->GetAttributeNS(NS_ConvertASCIItoUCS2(nsSOAPUtils::kSOAPEnvURI), 
                              NS_ConvertASCIItoUCS2(nsSOAPUtils::kEncodingStyleAttribute),
                              value);
      if (value.Length() > 0) {
        *aEncodingStyle = value.ToNewCString();
        break;
      }
    }
    nsCOMPtr<nsIDOMNode> temp = node;
    temp->GetParentNode(getter_AddRefs(node));
  }
}

JSContext*
nsSOAPUtils::GetSafeContext()
{
  // Get the "safe" JSContext: our JSContext of last resort
  nsresult rv;
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", 
                  &rv);
  if (NS_FAILED(rv))
    return nsnull;
  nsCOMPtr<nsIThreadJSContextStack> tcs = do_QueryInterface(stack);
  JSContext* cx;
  if (NS_FAILED(tcs->GetSafeJSContext(&cx))) {
    return nsnull;
  }
  return cx;
}
 
JSContext*
nsSOAPUtils::GetCurrentContext()
{
  // Get JSContext from stack.
  nsresult rv;
  NS_WITH_SERVICE(nsIJSContextStack, stack, "nsThreadJSContextStack", 
                  &rv);
  if (NS_FAILED(rv))
    return nsnull;
  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)))
    return nsnull;
  return cx;
}

nsresult 
nsSOAPUtils::ConvertValueToJSVal(JSContext* aContext, 
                                 nsISupports* aValue, 
                                 JSObject* aJSValue, 
                                 PRInt32 aType,
                                 jsval* vp)
{
  *vp = JSVAL_NULL;
  switch(aType) {
    case nsISOAPParameter::PARAMETER_TYPE_VOID:
      *vp = JSVAL_VOID;
      break;

    case nsISOAPParameter::PARAMETER_TYPE_STRING:
    {
      nsCOMPtr<nsISupportsWString> wstr = do_QueryInterface(aValue);
      if (!wstr) return NS_ERROR_FAILURE;
      
      nsXPIDLString data;
      wstr->GetData(getter_Copies(data));
      
      if (data) {
        JSString* jsstr = JS_NewUCStringCopyZ(aContext,
                                             (const jschar*)data);
        if (jsstr) {
          *vp = STRING_TO_JSVAL(jsstr);
        }
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_BOOLEAN:
    {
      nsCOMPtr<nsISupportsPRBool> prb = do_QueryInterface(aValue);
      if (!prb) return NS_ERROR_FAILURE;

      PRBool data;
      prb->GetData(&data);

      if (data) {
        *vp = JSVAL_TRUE;
      }
      else {
        *vp = JSVAL_FALSE;
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_DOUBLE:
    {
      nsCOMPtr<nsISupportsDouble> dub = do_QueryInterface(aValue);
      if (!dub) return NS_ERROR_FAILURE;

      double data;
      dub->GetData(&data);

      *vp = DOUBLE_TO_JSVAL((jsdouble)data);

      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_FLOAT:
    {
      nsCOMPtr<nsISupportsFloat> flt = do_QueryInterface(aValue);
      if (!flt) return NS_ERROR_FAILURE;

      float data;
      flt->GetData(&data);

      *vp = DOUBLE_TO_JSVAL((jsdouble)data);
      
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_LONG:
    {
      // XXX How to express 64-bit values in JavaScript?
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    case nsISOAPParameter::PARAMETER_TYPE_INT:
    {
      nsCOMPtr<nsISupportsPRInt32> isupint32 = do_QueryInterface(aValue);
      if (!isupint32) return NS_ERROR_FAILURE;

      PRInt32 data;
      isupint32->GetData(&data);
      
      *vp = INT_TO_JSVAL(data);
      
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_SHORT:
    {
      nsCOMPtr<nsISupportsPRInt16> isupint16 = do_QueryInterface(aValue);
      if (!isupint16) return NS_ERROR_FAILURE;

      PRInt16 data;
      isupint16->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);
      
      break;
    }
    
    case nsISOAPParameter::PARAMETER_TYPE_BYTE:
    {
      nsCOMPtr<nsISupportsChar> isupchar = do_QueryInterface(aValue);
      if (!isupchar) return NS_ERROR_FAILURE;

      char data;
      isupchar->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);

      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_ARRAY:
    {
      // XXX Can't (easily) convert a native nsISupportsArray
      // to a script array.
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    case nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_ARRAY:
    case nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_OBJECT:
    {
      *vp = OBJECT_TO_JSVAL(aJSValue);
      break;
    }
  }

  return NS_OK;
}

nsresult 
nsSOAPUtils::ConvertJSValToValue(JSContext* aContext,
                                 jsval val, 
                                 nsISupports** aValue,
                                 JSObject** aJSValue,
                                 PRInt32* aType)
{
  *aValue = nsnull;
  *aJSValue = nsnull;
  if (JSVAL_IS_NULL(val)) {
    *aType = nsISOAPParameter::PARAMETER_TYPE_NULL;
  }
  else if (JSVAL_IS_VOID(val)) {
    *aType = nsISOAPParameter::PARAMETER_TYPE_VOID;
  }
  else if (JSVAL_IS_STRING(val)) {
    JSString* jsstr;
    *aType = nsISOAPParameter::PARAMETER_TYPE_STRING;
    
    jsstr = JSVAL_TO_STRING(val);
    if (jsstr) {
      nsCOMPtr<nsISupportsWString> wstr = do_CreateInstance(NS_SUPPORTS_WSTRING_PROGID);
      if (!wstr) return NS_ERROR_FAILURE;

      PRUnichar* data = NS_REINTERPRET_CAST(PRUnichar*, 
                                            JS_GetStringChars(jsstr));
      if (data) {
        wstr->SetData(data);
      }
      *aValue = wstr;
      NS_ADDREF(*aValue);
    }
  }
  else if (JSVAL_IS_DOUBLE(val)) {
    *aType = nsISOAPParameter::PARAMETER_TYPE_DOUBLE;
    
    nsCOMPtr<nsISupportsDouble> dub = do_CreateInstance(NS_SUPPORTS_DOUBLE_PROGID);
    if (!dub) return NS_ERROR_FAILURE;

    dub->SetData((double)(*JSVAL_TO_DOUBLE(val)));
    *aValue = dub;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_INT(val)) {
    *aType = nsISOAPParameter::PARAMETER_TYPE_INT;
    
    nsCOMPtr<nsISupportsPRInt32> isupint = do_CreateInstance(NS_SUPPORTS_PRINT32_PROGID);
    if (!isupint) return NS_ERROR_FAILURE;
    
    isupint->SetData((PRInt32)JSVAL_TO_INT(val));
    *aValue = isupint;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_BOOLEAN(val)) {
    *aType = nsISOAPParameter::PARAMETER_TYPE_BOOLEAN;
    
    nsCOMPtr<nsISupportsPRBool> isupbool = do_CreateInstance(NS_SUPPORTS_PRBOOL_PROGID);
    if (!isupbool) return NS_ERROR_FAILURE;

    isupbool->SetData((PRBool)JSVAL_TO_BOOLEAN(val));
    *aValue = isupbool;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_OBJECT(val)) {
    JSObject* jsobj = JSVAL_TO_OBJECT(val);
    if (JS_IsArrayObject(aContext, jsobj)) {
      *aType = nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_ARRAY;
    }
    else {
      *aType = nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_OBJECT;
    }
    *aJSValue = jsobj;
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}
