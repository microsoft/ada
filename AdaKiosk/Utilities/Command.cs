// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows.Media;

namespace AdaKiosk
{
    public class Message
    {
        public string User;
        public string Text;
        public string FromGroup;
        public string Group;
        public Command[] Commands;

        public static Message FromJson(string json)
        {
            JsonTextReader reader = new JsonTextReader(new StringReader(json));
            while (reader.Read())
            {
                if (reader.TokenType == JsonToken.StartObject)
                {
                    return ReadMessage(reader);
                }
                else
                {
                    throw new Exception("Unexpected json token: " + reader.TokenType);
                }
            }

            return null;
        }

        private static Message ReadMessage(JsonTextReader reader)
        {
            Message result = new Message();
            while (reader.Read() && reader.TokenType != JsonToken.EndObject)
            {
                if (reader.TokenType == JsonToken.PropertyName)
                {
                    string name = reader.Value as string;
                    if (reader.Read())
                    {
                        if (reader.TokenType == JsonToken.String)
                        {
                            string value = reader.Value as string;
                            if (name == "fromUserId")
                            {
                                result.User = value;
                            }
                            else if (name == "data")
                            {
                                result.Text = value;
                            }
                            else if (name == "type")
                            {
                                // value should be "message".
                            }
                            else if (name == "from")
                            {
                                result.FromGroup = value;
                            }
                            else if (name == "group")
                            {
                                result.Group = value;
                            }
                            else if (name == "dataType")
                            {
                                // value should be "json"
                            }
                            else
                            {
                                Debug.WriteLine("missing json mapping for string property: " + name + " = " + value);
                            }
                        }
                        else if (reader.TokenType == JsonToken.StartArray)
                        {
                            if (name == "data")
                            {
                                result.Commands = Command.ReadCommandArray(reader);
                            }
                        }
                        else if (reader.TokenType == JsonToken.StartObject)
                        {
                            if (name == "data")
                            {
                                result.Commands = new Command[1] { Command.ReadCommand(reader) };
                            }
                        }
                    }
                }
            }
            return result;
        }
    }

    public class Pixel
    {
        public int Strip;
        public string Led;
        public Color Color;
    }

    public class ColumnColor
    {
        public int Index;
        public Color Color;
    }

    public class Command
    {
        public string Target;
        public string command;
        public string Name;
        public int Speed;
        public int Direction;
        public int Size;
        public int Strip;
        public int Index;
        public int Seconds;
        public int Iterations;
        public double F1;
        public double F2;
        public string Start;
        public int Sequence;
        public Color[] Colors;
        public ColumnColor[] Columns; // for ColumnFade
        public Pixel[] Pixels; // for SetPixels.

        internal static Command[] ReadCommandArray(JsonTextReader reader)
        {
            List<Command> result = new List<Command>();
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
            {
                result.Add(ReadCommand(reader));
            }
            return result.ToArray();
        }

        internal static Command ReadCommand(JsonTextReader reader)
        {
            Command result = new Command();
            while (reader.Read() && reader.TokenType != JsonToken.EndObject)
            {
                if (reader.TokenType == JsonToken.PropertyName)
                {
                    string name = reader.Value as String;
                    if (reader.Read())
                    {
                        if (reader.TokenType == JsonToken.String)
                        {
                            string value = reader.Value as string;
                            switch (name)
                            {
                                case "target":
                                    result.Target = value;
                                    break;
                                case "command":
                                    result.command = value;
                                    break;
                                case "name":
                                    result.Name = value;
                                    break;
                                case "start":
                                    result.Start = value;
                                    break;
                                default:
                                    Debug.WriteLine("missing json mapping for string property: " + name + " = " + value);
                                    break;
                            }
                        }
                        else if (reader.TokenType == JsonToken.Boolean)
                        {
                            bool value = (bool)reader.Value;
                            Debug.WriteLine("missing json mapping for boolean property: " + name + " = " + value);
                        }
                        else if (reader.TokenType == JsonToken.Float)
                        {
                            double value = (double)reader.Value;
                            switch (name)
                            {
                                case "f1":
                                    result.F1 = value;
                                    break;
                                case "f2":
                                    result.F2 = value;
                                    break;
                                default:
                                    Debug.WriteLine("missing json mapping for double property: " + name + " = " + value);
                                    break;
                            }
                        }
                        else if (reader.TokenType == JsonToken.Integer)
                        {
                            int i = (int)(Int64)reader.Value;
                            switch (name)
                            {
                                case "speed":
                                    result.Speed = i;
                                    break;
                                case "direction":
                                    result.Direction = i;
                                    break;
                                case "size":
                                    result.Size = i;
                                    break;
                                case "seconds":
                                    result.Seconds = i;
                                    break; ;
                                case "iterations":
                                    result.Iterations = i;
                                    break; ;
                                case "sequence":
                                    result.Sequence = i;
                                    break;
                                case "strip":
                                    result.Strip = i;
                                    break;
                                case "index":
                                    result.Index = i;
                                    break;
                                default:
                                    Debug.WriteLine("missing json mapping for integer property: " + name + " = " + i);
                                    break;
                            }
                        }
                        else if (reader.TokenType == JsonToken.StartArray)
                        {
                            if (name == "colors")
                            {
                                result.Colors = ReadColors(reader);
                            }
                            else if(name == "columns")
                            {
                                result.Columns = ReadColumns(reader);
                            }
                            else if (name == "pixels")
                            {
                                result.Pixels = ReadPixels(reader);
                            }
                            else
                            {
                                Debug.WriteLine("missing json mapping for array type property named: " + name);
                                SkipToEndArray(reader);
                            }
                        }
                        else
                        {
                            throw new Exception($"unexpected token type property value type {reader.TokenType} reading a Command");
                        }
                    }
                }
            }
            return result;
        }

        private static void SkipToEndArray(JsonTextReader reader)
        {
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
                ;
        }

        private static ColumnColor[] ReadColumns(JsonTextReader reader)
        {
            var result = new List<ColumnColor>();
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
            {
                if (reader.TokenType == JsonToken.StartObject)
                {
                    var c = ReadColumnColor(reader);
                    result.Add(c);
                }
                else
                {
                    throw new Exception("unexpected token type reading Columns array: " + reader.TokenType);
                }
            }
            return result.ToArray();
        }

        private static ColumnColor ReadColumnColor(JsonTextReader reader)
        {
            ColumnColor p = new ColumnColor();
            while (reader.Read() && reader.TokenType != JsonToken.EndObject)
            {
                if (reader.TokenType == JsonToken.PropertyName)
                {
                    string name = reader.Value as String;
                    if (reader.Read())
                    {
                        if (reader.TokenType == JsonToken.Integer)
                        {
                            if (name == "index")
                            {
                                p.Index = (int)(Int64)reader.Value;
                            }
                            else
                            {
                                throw new Exception("unexpected property found in Column color info: " + name);
                            }
                        }
                        else if (reader.TokenType == JsonToken.StartArray)
                        {
                            if (name == "color")
                            {
                                p.Color = ReadColor(reader);
                            }
                            else
                            {
                                throw new Exception("unexpected property found in Column color info: " + name);
                            }
                        }
                        else
                        {
                            throw new Exception("unexpected token type reading Column color info: " + reader.TokenType);
                        }
                    }
                }
            }
            return p;
        }

        private static Pixel[] ReadPixels(JsonTextReader reader)
        {
            List<Pixel> result = new List<Pixel>();
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
            {
                if (reader.TokenType == JsonToken.StartObject)
                {
                    Pixel p = ReadPixel(reader);
                    result.Add(p);
                }
                else
                {
                    throw new Exception("unexpected token type reading Pixel array: " + reader.TokenType);
                }
            }
            return result.ToArray();
        }

        private static Pixel ReadPixel(JsonTextReader reader)
        {
            Pixel p = new Pixel();
            while (reader.Read() && reader.TokenType != JsonToken.EndObject)
            {
                if (reader.TokenType == JsonToken.PropertyName)
                {
                    string name = reader.Value as String;
                    if (reader.Read())
                    {
                        if (reader.TokenType == JsonToken.Integer)
                        {
                            int i = (int)(Int64)reader.Value;
                            switch (name)
                            {
                                case "s":
                                    p.Strip = i;
                                    break;
                                default:
                                    Debug.WriteLine("missing json mapping for Pixel integer property named: " + name);
                                    break;
                            }
                        }
                        else if (reader.TokenType == JsonToken.String)
                        {
                            string i = (string)reader.Value;
                            switch (name)
                            {
                                case "l":
                                    p.Led = i;
                                    break;
                                default:
                                    Debug.WriteLine("missing json mapping for Pixel string property named: " + name);
                                    break;
                            }
                        }
                        else if (reader.TokenType == JsonToken.StartArray)
                        {
                            if (name == "color")
                            {
                                p.Color = ReadColor(reader);
                            }
                            else
                            {
                                Debug.WriteLine("missing json mapping for Pixel array: " + name);
                                SkipToEndArray(reader);
                            }
                        }
                        else
                        {
                            throw new Exception("unexpected token type reading Pixel data: " + reader.TokenType);
                        }
                    }
                }
            }
            return p;
        }

        private static Color[] ReadColors(JsonTextReader reader)
        {
            List<Color> result = new List<Color>();
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
            {
                if (reader.TokenType == JsonToken.StartArray)
                {
                    Color c = ReadColor(reader);
                    result.Add(c);
                }
                else
                {
                    throw new Exception("unexpected token type reading a color: " + reader.TokenType);
                }
            }
            return result.ToArray();
        }

        private static Color ReadColor(JsonTextReader reader)
        {
            // this tedious just for performance reasons, zero memory alloc.
            int r = 0;
            int g = 0;
            int b = 0;
            int state = 0;
            while (reader.Read() && reader.TokenType != JsonToken.EndArray)
            {
                if (reader.TokenType == JsonToken.Integer)
                {
                    int x = (int)(Int64)reader.Value;
                    switch (state)
                    {
                        case 0:
                            r = x;
                            break;
                        case 1:
                            g = x;
                            break;
                        case 2:
                            b = x;
                            break;
                        default:
                            break;
                    }
                    state++;
                }
                else
                {
                    throw new Exception("unexpected token type reading color components: " + reader.TokenType);
                }
            }
            return Color.FromRgb((byte)r, (byte)g, (byte)b);
        }
    }
}
