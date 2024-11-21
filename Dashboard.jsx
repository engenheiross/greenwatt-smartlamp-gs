import React, { useState, useEffect } from 'react';
import Plot from 'react-plotly.js';

import "../../dashboard.css"

export default function Dashboard() {
  const [intensityData, setIntensityData] = useState([]);
  const [luminosityData, setLuminosityData] = useState([]);
  const [motionData, setMotionData] = useState([]); 
  const [loading, setLoading] = useState(true); 
  const [error, setError] = useState(null); 
  const [lightsOn, setLightsOn] = useState(true);

  const ip = "34.55.39.20" // IP do Servidor Fiware
  const lampId = "003";
  const refreshRate = 5000; // em milisegundos  

  const mapToPercentage = (value) => {
    return (value / 255) * 100;
  };

  const mapRange = (value, inMin, inMax, outMin, outMax) => {
    return ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
  };


  const fetchData = async (attributeName, setState) => {
    const url = `http://${ip}:8666/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:${lampId}/attributes/${attributeName}?lastN=20`;
  
    const headers = {
      'fiware-service': 'smart',
      'fiware-servicepath': '/',
    };
  
    try {
      const response = await fetch(url, { headers });
      if (!response.ok) throw new Error(`Failed to fetch ${attributeName} data`);
      let result = await response.json();
      result = result['contextResponses'][0]['contextElement']['attributes'][0]['values'];
      let resultData = result.map(item => [item.attrValue, item.recvTime]);


      switch(attributeName) {
        case 'intensity':
          resultData = resultData.map(item => [Math.round(mapRange(item[0], 0, 255, 0, 100)), item[1]]);
          break;
        case 'luminosity':

          resultData = resultData.map(item => [Math.round(mapRange(item[0], 32, 4063, 0, 100)), item[1]]);

          break;
        case 'motion':
          if (resultData[resultData.length - 1][0] == 0) {
            setLightsOn(false);
          }
          else {
            setLightsOn(true);
          }
          
          break;
        

        default:
          break;
      }

      setState(resultData); // Dynamically update the appropriate state
      setLoading(false);
    } catch (err) {
      setError(err.message);
      setLoading(false);
    }
  };

  const sendToggleRequest = async () => {
    const url = `http://${ip}:1026/v2/entities/urn:ngsi-ld:Lamp:${lampId}/attrs/`;

    const body = {
      [lightsOn ? "off" : "on"]: {
        type: "command",
        value: ""
      }
    };
    
    const headers = {
      "fiware-service": "smart",
      "fiware-servicepath": "/",
      "Content-Type": "application/json",
    };

    try {
      const response = await fetch(url, {
        method: "PATCH", // HTTP method
        headers: headers, // Headers
        body: JSON.stringify(body), // Body as JSON string
      });
  
      if (!response.ok) {
        throw new Error(`Error: ${response.status} - ${response.statusText}`);
      }
    } catch (error) {
      console.error("Error sending PATCH request:", error.message);
    }
  };
  

  // Fetch intensityData periodically
  useEffect(() => {

    const fetchAllData = () => {
      fetchData("intensity", setIntensityData);
      fetchData("luminosity", setLuminosityData);
      fetchData("motion", setMotionData);
    }
    fetchAllData();

    // Set up polling every 5 seconds
    const interval = setInterval(() => {
      fetchAllData();
    }, refreshRate);

    // Cleanup interval on unmount
    return () => clearInterval(interval);
  }, []); // Empty dependency array ensures it runs once on mount

  return (
    <div className="flex flex-col items-center justify-center w-full min-h-screen max-w-[1440px] mx-auto p-8">
      {loading && <p>Loading Data...</p>}
      {error && <p>Error: {error}</p>}

      <img onClick={sendToggleRequest}
      src="https://static.thenounproject.com/png/1841400-200.png" 
      className={`button w-36 rounded-full cursor-pointer ${lightsOn ? "bg-red-400" : "bg-red-900"}`} 
      />


      <div className="flex flex-col items-center justify-center">
<Plot
  data={[
    {
      x: intensityData.map(item => item[1]), // Time values (Intensity)
      y: intensityData.map(item => item[0]), // Attribute values (Intensity)
      type: 'scatter',
      mode: 'lines',
      marker: { color: 'blue' },
      name: 'Intensidade', // Name for legend
    },
    {
      x: luminosityData.map(item => item[1]), // Time values (Luminosity)
      y: luminosityData.map(item => item[0]), // Attribute values (Luminosity)
      type: 'scatter',
      mode: 'lines',
      line: { color: 'green', width: 1, opacity: 0.3 }, // Transparency set here (0.3 for less visible line)
      name: 'Luz Natural', // Name for legend
    },
  ]}
  layout={{
    title: 'Lâmpada: Intensidade e Luz Natural',
    xaxis: {
      title: 'Tempo',
      type: 'date',
    },
    yaxis: {
      title: 'Valor',
    },
    showlegend: true, // Display legend
  }}
/>

      </div>
    </div>
  );
}
